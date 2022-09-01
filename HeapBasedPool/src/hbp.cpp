#include <memory>

#include "hbp.h"
#include "Handle.h"

namespace hbp
{
	//-----------------------------------------------------------
	void* FreeListStorage::malloc(CSize size)
	{
		if (size == 0u)
			return nullptr;

		Size minS = size < s_ptrSize ? s_ptrSize : size;
		Size mod = s_ptrSize % minS;
		Size actualSize = minS
			+ (mod ? s_ptrSize - mod : 0)
			+ s_ptrSize;

		void* curSeg = m_data;
		void* prevSeg = _tryMallocN(curSeg, actualSize);

		if (curSeg) {
			void* newBlockStart = nullptr;
			if (_segSize(curSeg) - actualSize != 0u || 
				_nextSeg(curSeg)) {
				newBlockStart = static_cast<char*>(curSeg) + actualSize;
				_segSize(newBlockStart) = _segSize(curSeg) - actualSize;
			}
			if (prevSeg && prevSeg != curSeg) {
				_nextSeg(prevSeg) = newBlockStart;
			} else if (curSeg == m_data) {
				m_data = newBlockStart;
			}
			_segSize(curSeg) = actualSize;

#if HEAP_BASED_POOL_ENABLE_MEM_LOG
			_log(static_cast<char*>(curSeg) + s_ptrSize, actualSize , true);
#endif

			return static_cast<char*>(curSeg) + s_ptrSize;
		}

		return nullptr;
	}
	
	//-----------------------------------------------------------
	void FreeListStorage::free(void* ptr)
	{
		if (!ptr)
			return;

		void* actualAddres = static_cast<char*>(ptr) - s_ptrSize;
#if HEAP_BASED_POOL_ENABLE_MEM_LOG
		_log(static_cast<char*>(ptr), *static_cast<size_t*>(actualAddres), false);
#endif
		addBLock(actualAddres, _segSize(actualAddres));
	}

	//-----------------------------------------------------------
	void FreeListStorage::addBLock(void* ptr, CSize size)
	{
		void* pos = _findPrev(ptr);
		if (!pos)
			m_data = _makeList(ptr, m_data, size);
		else {
			void* nextSeg = _makeList(ptr, _nextSeg(pos), size);
			if (static_cast<char*>(pos) + _segSize(pos) == nextSeg) {
				_segSize(pos) += _segSize(nextSeg);
			} else {
				_nextSeg(pos) = nextSeg;
			}
		}
	}

	//-----------------------------------------------------------
	void* FreeListStorage::_findPrev(void* const ptr) const
	{
		if (!m_data || ptr < m_data)
			return nullptr;

		void* iter = m_data;
		void* next = nullptr;
		while (true) {
			next = _nextSeg(iter);
			if (!next || ptr < next)
				return iter;
			iter = next;
		}
	}

	//-----------------------------------------------------------
	void FreeListStorage::reinit(FreeListStorage* ptr)
	{
		m_data = ptr ? ptr->m_data : nullptr;
	}

	//defragmentation functionality
	
	//-----------------------------------------------------------
	void FreeListStorage::getNextHole(void*& nextHole, Size& holeSize, void* start /*nullptr*/) const
	{
		nextHole = start != nullptr ? _nextSeg(start) : m_data;
		if (nextHole)
			holeSize = _segSize(nextHole);
	}

#if  HEAP_BASED_POOL_ENABLE_MEM_LOG
	//-----------------------------------------------------------
	void FreeListStorage::_log(const void* const ptr, CSize blockNum, const bool isAllocation)
	{
		//memory location
		//size of allocated memory in blocks
		printf_s("*************************************************************\n");
		printf_s("At memory location[%p] has been %s [%zu] bytes of memory\n",
			ptr,
			isAllocation ? "allocated" : "freed",
			blockNum);
	}
#endif// HEAP_BASED_POOL_ENABLE_MEM_LOG

	//-----------------------------------------------------------
	void* FreeListStorage::_makeList(void* const begin, void* const end, CSize size)
	{
		Size totalSize = size;
		void* nextSeg = end;
		if (static_cast<char*>(begin) + size == end) {
			totalSize += _segSize(end);
			nextSeg = _nextSeg(end);
		}
		_segSize(begin) = totalSize;
		_nextSeg(begin) = nextSeg;
		return begin;
	}

	//-----------------------------------------------------------
	void* FreeListStorage::_tryMallocN(void*& begin, Size n)
	{
		Size segSize = 0u;
		void* prev = nullptr;
		while(begin) {
			segSize = _segSize(begin);
			if (segSize >= n)
				break;
			prev = begin;
			begin = _nextSeg(begin);
		}
		return prev;
	}

	//-----------------------------------------------------------
	HeapStorage::HeapStorage()
		:m_data{ nullptr }
		,m_currentSize{ 0u }
		,m_maxSize{ 0u }	
	{
	}

	//-----------------------------------------------------------
	HeapStorage::~HeapStorage()
	{
		if (m_data) {
			std::free(m_data);
			m_data = nullptr;
		}
	}

	//-----------------------------------------------------------
	void HeapStorage::init(CSize size)
	{
		if (m_data) {
			printf_s("HeapStorage is already initialized!\n");
			return;
		}

		m_data = std::malloc(size);
		if (!m_data) {
			printf_s("Bad alloc, malloc has returned nullptr!\n");
			return;
		}

		m_storage.addBLock(m_data, size);
		m_maxSize = size;
	}

	//-----------------------------------------------------------
	void* HeapStorage::malloc(CSize size)
	{
		if (!m_data) {
			printf_s("Heap storage isn't initialized!\n");
			return nullptr;
		} else if (size + s_ptrSize > m_maxSize - m_currentSize) {
			if (!_reinit(size + s_ptrSize + s_ptrSize)) {/*obj size + meta data about obj + metadata about FreeListStorage*/

				printf_s("There isn't enough space in HeapStorage. Required spase is[%zu], and current space is[%zu], and max space is[%zu]\n",
					size + s_ptrSize, m_currentSize, m_maxSize);
				return nullptr;
			}
		}

		void* res = m_storage.malloc(size);
		if (res){
			Size oS = m_storage.getObjSizeInBytes(res) + s_ptrSize;
			m_currentSize += oS;
		} else if (_canDefragment(size + s_ptrSize)) {
			//try defragment
			_defragment();
			res = m_storage.malloc(size);
			if (!res) {
				printf_s("Bad alloc after defragmetation, malloc has returned nullptr!\n");
				DEBUG_DumpAllFreeMemory();
				return res;
			}
			m_currentSize += m_storage.getObjSizeInBytes(res) + s_ptrSize;
		}
		return res;
	}

	//-----------------------------------------------------------
	void HeapStorage::free(void* ptr)
	{
		if (!ptr || m_currentSize == 0u) 
			return;
		
		m_currentSize -= m_storage.getObjSizeInBytes(ptr) + s_ptrSize;
		m_storage.free(ptr);
	}

	//-----------------------------------------------------------
	void HeapStorage::DEBUG_DumpAllFreeMemory()
	{
#if not defined (NDEBUG)
		void* nextHole;
		void* start = nullptr;
		Size holeSize;
		printf_s("*************************************************************\n");
		printf_s("[DEBUG] Dump all free memory\n");
		do {
			m_storage.getNextHole(nextHole, holeSize, start);
			if (nextHole)
			{
				printf_s("[DEBUG] Memory block start [0x%p] -> memory block end [0x%p], block size [%zu]\n", 
					nextHole, static_cast<char*>(nextHole) + holeSize, holeSize);
				start = nextHole;
			}
		} while (nextHole != nullptr);
		printf_s("[DEBUG] Current size[%zu], Max size[%zu]\n", m_currentSize, m_maxSize);
		printf_s("*************************************************************\n");
#endif
	}

	//-----------------------------------------------------------
	void HeapStorage::cleanAll()
	{
		if (m_data) {
			std::free(m_data);
			m_data = nullptr;
			m_currentSize = m_maxSize = 0u;
			m_storage.reinit(nullptr);
		}
	}

	//-----------------------------------------------------------
	bool HeapStorage::_reinit(CSize requestedSize)
	{
		//calculate new size
		Size newMaxSize = m_maxSize;
		constexpr CSize maxSize = std::numeric_limits<Size>::max();

		while (requestedSize > newMaxSize - m_currentSize &&
			maxSize - newMaxSize > newMaxSize / 2)
		{
			newMaxSize = newMaxSize + newMaxSize / 2;
		}
		
		void* newData = nullptr;

		if (maxSize - newMaxSize < newMaxSize / 2 ||
			requestedSize > newMaxSize - m_currentSize) {
			printf_s("Cannot add more memory, it will exceed limit!"
				"Current size[%zu], MaxSize[%zu], RequestedSize[%zu]\n", 
				m_currentSize, m_maxSize, requestedSize);
			return false;
		}
		
		newData = std::malloc(newMaxSize);
		
		if (!newData) {
			printf_s("Bad alloc, malloc has returned nullptr!\n");
			return false;
		}

		FreeListStorage newStorage;
		newStorage.addBLock(newData, newMaxSize);
		
		HandleManager::iterator b = GetHandleManager().begin();
		HandleManager::iterator e = GetHandleManager().end();

		for (; b != e; b++)	{

			Size bytes = m_storage.getObjSizeInBytes(b->second);
			void* obj = newStorage.malloc(bytes);
			std::memcpy(obj, b->second, bytes);
			b->second = obj;
		}
		
		if (m_data) {
			std::free(m_data);
			m_data = newData;
		}

		m_storage.reinit(&newStorage);

		m_maxSize = newMaxSize;
		return true;
	}

	//-----------------------------------------------------------
	void HeapStorage::_defragment()
	{
		//1. find hole
		void* hole;
		void* start = nullptr;
		Size holeSize = 0u;

		while (m_storage.getNextHole(hole, holeSize, start), hole != nullptr) {

			//2. find allocated block to the rigth of the hole
			void* obj = nullptr;
			Size objSize = 0u;

			HandleManager::iterator b = GetHandleManager().begin();
			HandleManager::iterator e = GetHandleManager().end();

			for (; b != e; b++) {
				if (b->second > hole && 
					(objSize = m_storage.getObjSizeInBytes(b->second)) <= holeSize) {
					obj = b->second;
					break;
				}
			}

			if (b == e)
			{
				start = hole;
				continue;
			}

			//3. swap hole with object
			if (obj) {
				void* newObj = m_storage.malloc(objSize);
				::memcpy(newObj, obj, objSize);
				b->second = newObj;
				m_storage.free(obj);
			}
		}
	}

	HeapStorage g_heapStorage{};

	//-----------------------------------------------------------
	HeapStorage& GetHeapStorage()
	{
		return g_heapStorage;
	}

	//-----------------------------------------------------------
	HeapStorageHandles::HeapStorageHandles()
	{
	}

	//-----------------------------------------------------------
	HeapStorageHandles::~HeapStorageHandles()
	{
	}

	//-----------------------------------------------------------
	IHandle* HeapStorageHandles::malloc(CSize size)
	{
		void* res = HeapStorage::malloc(size);
		return helpers::makeHandle(res);
	}

	//-----------------------------------------------------------
	IHandle* HeapStorageHandles::malloc_n(CSize size, CSize handlesNumber)
	{
		void* res = HeapStorage::malloc(size * handlesNumber);
		return helpers::makeHandle(res, handlesNumber, size);
	}

	//-----------------------------------------------------------
	void HeapStorageHandles::free(IHandle* ptr)
	{
		if (!ptr)
			return;
//		HeapStorage::free(helpers::destroyHandle(ptr));
	}

	//-----------------------------------------------------------
	/*
	<precondition>
	ptr has to be a pointer to the first element is sequnce(array, vector)
	*/
	void HeapStorageHandles::free_n(IHandle* ptr, CSize handlesNumber)
	{
		if (!ptr)
			return;
		for (Size i = 0; i < handlesNumber; i++)
		{
			//HeapStorage::free(helpers::destroyHandle(ptr + i));
		}
	}

	//-----------------------------------------------------------
	void handleRelease(void* ptr)
	{
		GetHeapStorage().free(ptr);
	}

}//namaspace hbp