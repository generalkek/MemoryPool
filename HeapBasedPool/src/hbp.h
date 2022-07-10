#ifndef HEAP_BASED_POOL_SRC_HBP
#define HEAP_BASED_POOL_SRC_HBP

#if defined(NDEBUG) || defined(DISABLE_LOG)
#define HEAP_BASED_POOL_ENABLE_MEM_LOG 0
#else
#define HEAP_BASED_POOL_ENABLE_MEM_LOG  1
#endif

#include "Handle.h"

namespace hbp
{
	typedef size_t Size;
	typedef const size_t CSize;
	
	constexpr static size_t s_ptrSize = sizeof(void*);

	class FreeListStorage
	{
	public:
								FreeListStorage() : m_data{ nullptr } {}
								~FreeListStorage() {}

		void*					malloc(CSize size);
		void					free(void* ptr);

		void					addBLock(void* ptr, CSize size);
		void					reinit(void* ptr, CSize size);
	private:

#ifdef HEAP_BASED_POOL_ENABLE_MEM_LOG
		void					_log(const void* const ptr, CSize blockNum, const bool isAllocation);
#endif// HEAP_BASED_POOL_ENABLE_MEM_LOG

		void*					_findPrev(void* const ptr) const;

		static void*			_makeList(void* const begin, void* const end, CSize size);
		static void*			_tryMallocN(void*& begin, Size n);
		
		inline static void*&	_nextOf(void* const p)
		{
			return *static_cast<void**>(p);
		}

	private:
			void* m_data;
	};

	class HeapStorage 
	{
	public:
								HeapStorage();
		virtual					~HeapStorage();

		/*
		Take into account that memory overhead is up to 2 in the worse case(for types
		which size is equal or less than sizeof(void*))
		*/
		void					init(CSize size);
		void*					malloc(CSize size);
		void					free(void* ptr);

	private:
		void*			m_data;
		FreeListStorage m_storage;
	};

	class HeapStorageHandles : public HeapStorage
	{
	public:
								HeapStorageHandles();
		virtual					~HeapStorageHandles();

		Handle*					malloc(CSize size);
		void					free(Handle* ptr);

	};

}//namespace hbp
#endif //HEAP_BASED_POOL_SRC_HBP