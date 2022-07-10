#include "hbp.h"
#include <memory>

namespace hbp
{
	//-----------------------------------------------------------
	void* FreeListStorage::malloc(CSize size)
	{
		size_t blockNum = size / s_ptrSize	// number of blocks
			+ (size % s_ptrSize ? 1 : 0)	// for aligment
			+ 1;							// for metadata
		void* start = &(m_data);
		void* iter;

		do {
			if (!_nextOf(start))
				return nullptr;

			iter = _tryMallocN(start, blockNum);
		} while (!iter);

		void* meta = _nextOf(start);
		(*static_cast<size_t*>(meta)) = blockNum;

		_nextOf(start) = _nextOf(iter);

#ifdef HEAP_BASED_POOL_ENABLE_MEM_LOG
		_log(static_cast<char*>(meta) + s_ptrSize, blockNum, true);
#endif
		return static_cast<char*>(meta) + s_ptrSize;
	}

	//-----------------------------------------------------------
	void FreeListStorage::free(void* ptr)
	{
		void* actualAddres = static_cast<char*>(ptr) - s_ptrSize;

#ifdef HEAP_BASED_POOL_ENABLE_MEM_LOG
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
	void FreeListStorage::reinit(void* ptr, CSize size)
	{
		_makeList(ptr, m_data, size);
	}

	//-----------------------------------------------------------
	void* FreeListStorage::_findPrev(void* const ptr) const
	{
		if (!m_data || ptr < m_data)
			return nullptr;

		void* iter = _nextOf(m_data);

		while (true)
		{
			if (!_nextOf(iter) || ptr < iter)
				return iter;
			iter = _nextOf(iter);
		}
	}
#ifdef  HEAP_BASED_POOL_ENABLE_MEM_LOG
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
		int _n = 1;
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
	}

	//-----------------------------------------------------------
	void* HeapStorage::malloc(CSize size)
	{
		return m_storage.malloc(size);
	}

	//-----------------------------------------------------------
	void HeapStorage::free(void* ptr)
	{
		m_storage.free(ptr);
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
	Handle* HeapStorageHandles::malloc(CSize size)
	{
		void* res = HeapStorage::malloc(size + sizeof(Handle));
		Handle* h = static_cast<Handle*>(static_cast<void*>(static_cast<char*>(res) + size));
		new (h) Handle{ res };
		return h;
	}

	//-----------------------------------------------------------
	void HeapStorageHandles::free(Handle* ptr)
	{
		void* rawPtr = ptr->operator*<void>();
		ptr->~Handle();
		HeapStorage::free(rawPtr);
	}

}//namaspace hbp