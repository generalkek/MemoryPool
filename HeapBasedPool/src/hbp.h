#ifndef HEAP_BASED_POOL_SRC_HBP
#define HEAP_BASED_POOL_SRC_HBP

#if defined(NDEBUG) || defined(DISABLE_LOG)
#define HEAP_BASED_POOL_ENABLE_MEM_LOG 0
#else
#define HEAP_BASED_POOL_ENABLE_MEM_LOG  1
#endif

namespace hbp
{
	class FreeListStorage
	{
	public:
								FreeListStorage();
								~FreeListStorage();

		void*					malloc(size_t size);
		void					free(void* ptr);

		void					addBLock(void* ptr, size_t size);

	private:

		void*					_findPrev(void* const ptr) const;
		void*					_makeList(void* const begin, void* const end, size_t size);
		inline static void*&	_nextOf(void* const p)
		{
			return *static_cast<void**>(p);
		}

	private:
			void* m_data;
	};

}//namespace hbp
#endif //HEAP_BASED_POOL_SRC_HBP

