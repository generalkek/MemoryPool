#ifndef HEAP_BASED_POOL_SRC_HBP
#define HEAP_BASED_POOL_SRC_HBP

#if defined(NDEBUG) || defined(DISABLE_LOG)
#define HEAP_BASED_POOL_ENABLE_MEM_LOG 0
#else
#define HEAP_BASED_POOL_ENABLE_MEM_LOG  1
#endif

namespace hbp
{
	typedef size_t Size;
	typedef const size_t CSize;
	
	constexpr static size_t s_ptrSize = sizeof(void*);

	class FreeListStorage
	{
	public:
								FreeListStorage() 
									: m_data{ nullptr }
								{}
								~FreeListStorage() {}

		void*					malloc(CSize size);
		void					free(void* ptr);

		void					addBLock(void* ptr, CSize size);
		void					reinit(FreeListStorage* ptr);

		inline CSize			getObjSizeInBlocks(void* ptr) const 
		{
			return getObjSizeInBytes(ptr) / s_ptrSize;
		}

		inline CSize			getObjSizeInBytes(void* ptr) const
		{
			return _segSize(static_cast<char*>(ptr) - s_ptrSize) - s_ptrSize;
		}

	public:
		//defragmentation functionality
		void					getNextHole(void*& nextHole, Size& holeSize, void* start = nullptr) const;
		
	private:

#if HEAP_BASED_POOL_ENABLE_MEM_LOG
		void					_log(const void* const ptr, CSize blockNum, const bool isAllocation);
#endif// HEAP_BASED_POOL_ENABLE_MEM_LOG

		void*					_findPrev(void* const ptr) const;

		static void*			_makeList(void* const begin, void* const end, CSize size);

		static void*			_tryMallocN(void*& begin, Size n);
		
		inline static void*&	_nextOf(void* const p)
		{
			return *static_cast<void**>(p);
		}

		inline static Size&		_segSize(void* const p)
		{
			return *static_cast<Size*>(p);
		}

		inline static void*&	_nextSeg(void* const p)
		{
			return _nextOf(static_cast<char*>(p) + _segSize(p) - s_ptrSize);
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

		void					DEBUG_DumpAllFreeMemory();

		void					cleanAll();

	private:

		bool					_reinit(CSize requestedSize);
		
		inline bool				_canDefragment(CSize size) const { return m_maxSize - m_currentSize >= size; }
		void					_defragment();

	private:
		FreeListStorage m_storage;
		void*			m_data;
		Size			m_currentSize;
		Size			m_maxSize;
	};

	HeapStorage& GetHeapStorage();

	//forward declarations
	class IHandle;
	template<typename T>
	class Handle;

	class HeapStorageHandles : public HeapStorage
	{
	public:
								HeapStorageHandles();
		virtual					~HeapStorageHandles();

		void					free(IHandle* ptr);
		void					free_n(IHandle* ptr, CSize handlesNumber);

		template<typename T>
		Handle<T>*				allocHandle(CSize handlesNumber = 1)
		{
			IHandle* h = HeapStorageHandles::malloc(sizeof(T));
			return static_cast<Handle<T>*>(h);
		}
	private:
		IHandle*				malloc(CSize size);
		IHandle*				malloc_n(CSize size, CSize handleNumber);
	};
}//namespace hbp
#endif //HEAP_BASED_POOL_SRC_HBP