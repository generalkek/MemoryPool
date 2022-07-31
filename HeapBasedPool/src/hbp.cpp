#include <memory>

#include "hbp.h"
#include "Handle.h"

namespace hbp
{
	//-----------------------------------------------------------
	void* FreeListStorage::malloc(CSize size)
	{
		size_t blockNum = size / s_ptrSize	// number of blocks
			+ (size % s_ptrSize ? 1 : 0)	// for aligment
			+ 1;							// for metadata
		void* start = &m_data;
		void* iter;

		do {
			if (!_nextOf(start))
				return nullptr;

			iter = _tryMallocN(start, blockNum);
		} while (!iter);

		void* meta = _nextOf(start);
		(*static_cast<size_t*>(meta)) = blockNum;

		_nextOf(start) = _nextOf(iter);

#if HEAP_BASED_POOL_ENABLE_MEM_LOG
		_log(static_cast<char*>(meta) + s_ptrSize, blockNum, true);
#endif
		return static_cast<char*>(meta) + s_ptrSize;
	}

	//-----------------------------------------------------------
	void FreeListStorage::free(void* ptr)
	{
		void* actualAddres = static_cast<char*>(ptr) - s_ptrSize;

#if HEAP_BASED_POOL_ENABLE_MEM_LOG
		_log(static_cast<char*>(ptr), *static_cast<size_t*>(actualAddres), false);
#endif
		addBLock(actualAddres, *static_cast<size_t*>(actualAddres) * s_ptrSize);
	}

	//-----------------------------------------------------------
	void FreeListStorage::addBLock(void* ptr, CSize size)
	{
		void* pos = _findPrev(ptr);
		if (!pos)
			m_data = _makeList(ptr, m_data, size);
		else
			_nextOf(pos) = _makeList(ptr, _nextOf(pos), size);
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
		if (!start) {
			nextHole = m_data;
			holeSize = getHoleSizeInBlocks(&nextHole);
		} else {
			holeSize = getHoleSizeInBlocks(&start);
			nextHole = _nextOf(static_cast<char*>(start) + holeSize * s_ptrSize);
		}
	}

	//-----------------------------------------------------------
	CSize FreeListStorage::getHoleSizeInBlocks(void* ptr) const
	{
		void* b = _nextOf(ptr);
		void* next;
		Size n = 0u;
		do {
			next = _nextOf(b);
			if (next != (static_cast<char*>(b) + s_ptrSize))
				break;
			n++;
			b = next;
		} while (next);
		return n;
	}

	void FreeListStorage::swap(void* dest, void* src, CSize size)
	{
		::memcpy(dest, src, size);
	}

	//-----------------------------------------------------------
	void* FreeListStorage::_findPrev(void* const ptr) const
	{
		if (!m_data || ptr < m_data)
			return nullptr;

		void* iter = _nextOf(m_data);

		while (true) {
			if (!_nextOf(iter) || ptr < iter)
				return iter;
			iter = _nextOf(iter);
		}
	}
#if  HEAP_BASED_POOL_ENABLE_MEM_LOG
	//-----------------------------------------------------------
	void FreeListStorage::_log(const void* const ptr, CSize blockNum, const bool isAllocation)
	{
		//memory location
		//size of allocated memory in blocks
		printf_s("*************************************************************\n");
		printf_s("At memory location[%p] has been %s [%d] blocks of memory\n",
			ptr,
			isAllocation ? "allocated" : "freed",
			blockNum);
	}
#endif// HEAP_BASED_POOL_ENABLE_MEM_LOG

	//-----------------------------------------------------------
	void* FreeListStorage::_makeList(void* const begin, void* const end, CSize size)
	{
		char* old = static_cast<char*>(begin) + size - s_ptrSize;

		_nextOf(old) = end;

		if (begin == old)
			return begin;

		for (char* iter = old - s_ptrSize;
			iter != begin;
			old = iter, iter -= s_ptrSize) {
			_nextOf(iter) = old;
		}

		_nextOf(begin) = old;

		return begin;
	}

	//-----------------------------------------------------------
	void* FreeListStorage::_tryMallocN(void*& begin, Size n)
	{
		void* b = _nextOf(begin);
		while (--n != 0) {
			void* next = _nextOf(b);
			if (next != (static_cast<char*>(b) + s_ptrSize)) {
				begin = b;
				return nullptr;
			}
			b = next;
		}
		return b;
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
		void* res = m_storage.malloc(size);
		m_currentSize += size + s_ptrSize;
		return res;
	}

	//-----------------------------------------------------------
	void HeapStorage::free(void* ptr)
	{
		m_currentSize -= m_storage.getObjSizeInBytes(ptr) + s_ptrSize;
		m_storage.free(ptr);
	}

	//-----------------------------------------------------------
	bool HeapStorage::_reinit()
	{
		Size newSize = m_maxSize + m_maxSize / 2;
		void* newData = nullptr;

		if (newSize == 0) {
			printf_s("Heap is empty, so defragmentation is not needed!\n");
			return false;
		}
		
		newData = std::malloc(newSize);
		
		FreeListStorage newStorage;
		newStorage.addBLock(newData, newSize);
		
		HandleManager::iterator b = GetHandleManager().begin();
		HandleManager::iterator e = GetHandleManager().end();

		for (; b != e; b++)	{

			Size bytes = m_storage.getObjSizeInBytes(b->second);
			void* obj = newStorage.malloc(bytes);
			std::memcpy(obj, b->second, bytes);
			b->second = obj;
		}
		
		m_storage.reinit(&newStorage);

		if (m_data) {
			std::free(m_data);
			m_data = newData;
		}

		m_maxSize = newSize;
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
					(objSize = m_storage.getObjSizeInBlocks(b->second)) <= holeSize) {
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
				::memcpy(newObj, obj, objSize * s_ptrSize);
				b->second = newObj;
				m_storage.free(obj);
			}
		}
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
		HeapStorage::free(helpers::destroyHandle(ptr));
	}

	//-----------------------------------------------------------
	/*
	<precondition>
	ptr has to be a pointer to the first element is sequnce(array, vector)
	*/
	void HeapStorageHandles::free_n(IHandle* ptr, CSize handlesNumber)
	{
		for (Size i = 0; i < handlesNumber; i++)
		{
			HeapStorage::free(helpers::destroyHandle(ptr + i));
		}
	}
}//namaspace hbp