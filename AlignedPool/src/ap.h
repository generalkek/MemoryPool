#ifndef ALIGNED_POOL_SRC_AP
#define ALIGNED_POOL_SRC_AP

#if defined(NDEBUG) || defined(DISABLE_LOG)
#define ALIGNED_POOL_ENABLE_MEM_LOG 0
#else
#define ALIGNED_POOL_ENABLE_MEM_LOG  1
#endif

#define APM_POOL_NUMBER 16
#define APM_HIT_COUNT_TO_BE_CACHED 3
#define APM_ENABLE_CACHING 1

namespace align_pool
{
#if APM_ENABLE_CACHING
	class AlignedPoolManager;
#endif//APM_ENABLE_CACHING
	
	struct AlignedPool
	{
	public:
		/*
		first parameter is a blockSize
		second parameter is the number of blocks of blockSize size
		*/
		explicit		AlignedPool(size_t blockSize, size_t blockCount);
		explicit		AlignedPool(size_t blockSize, size_t blockCount, char* ptr);
						~AlignedPool();

		//delete copy constuctor and copy assigment
						AlignedPool(const AlignedPool& other) = delete;
		AlignedPool&	operator=(const AlignedPool& other) = delete;

		//custom move constructor and assignment
						AlignedPool(AlignedPool&& other) noexcept;
		AlignedPool&	operator=(AlignedPool&& other) noexcept;

		void*			malloc();
		void*			malloc_n(const size_t blockNumber);
		void			free(const void* ptr);
		void			free_n(const void* ptr, size_t blockNumber);
		
		inline bool		isFrom(const void* const ptr) const
														{
															return ptr >= m_data && ptr < static_cast<char*>(m_data) + m_blockSize * m_blockCount;
														}
	private:
		void			_init();
		inline void*	_getData(const size_t idx)		const;
		size_t			_getNextFreeIdx(const size_t)	const;
		size_t			_tryMallocN(size_t n)			const;
		size_t			_findIdx(const void* p)			const;

#if ALIGNED_POOL_ENABLE_MEM_LOG
	private:
		enum class MemHint
		{
			INFO = 0,
			ALLOC = 1 << INFO,
			FREE = 1 << ALLOC,
		};
		void			_log(int blockId, size_t memory, MemHint hint) const;
#endif//ALIGNED_POOL_ENABLE_MEM_LOG

#if APM_ENABLE_CACHING
		
		friend AlignedPoolManager;
		inline void			_fillRange(void*& b, void*& e) const;
#endif//APM_ENABLE_CACHING
	private:
		void*			m_data;
		size_t*			m_dataState;
		size_t			m_curFreeIdx;
		size_t			m_blockSize;
		size_t			m_blockCount;

	private:
		static constexpr size_t INVALID_ID = ~0u;
	};

	class AlignedPoolManager
	{
	public:
		//prevent the creation of large numbers of pools
		
		AlignedPoolManager();
		~AlignedPoolManager();

		AlignedPoolManager(const AlignedPoolManager&) = delete;
		AlignedPoolManager& operator=(const AlignedPoolManager&) = delete;

		AlignedPoolManager(AlignedPoolManager&& other) noexcept;
		AlignedPoolManager& operator=(AlignedPoolManager&& other) noexcept;

		/*
		actualy allocates memory for all pools, which are added
		*/
		void	init();
		bool	isInitialized() const { return m_data != nullptr; }

		void	addPool(size_t blockSize, size_t blockNum);
		void	removePool(size_t blockSize, size_t blockNum);

		void*	malloc(size_t size);
		void*	malloc_n(size_t size, size_t blockNumber);
		
		void	free(const void* ptr);
		void	free_n(const void* ptr, size_t blockNumber);
	private:
		struct PoolInfo
		{
			PoolInfo() : blockSize{ 0u }, blockNumber{ 0u }, pool{ nullptr } {}
			PoolInfo(PoolInfo&& other) noexcept;
			PoolInfo& operator=(PoolInfo&& other) noexcept;

			size_t			blockSize;
			size_t			blockNumber;
			AlignedPool*	pool;
		};
#if APM_ENABLE_CACHING
		struct Cache
		{
			Cache() : id{ ~0u }, hits{ 0u } {}
			size_t id;
			size_t hits;

			union
			{
				size_t size;
				struct
				{
					void* begin;
					void* end;
				};
			};
		};
		Cache				m_cacheMalloc;
		Cache				m_cacheFree;
#endif//APM_ENABLE_CACHING
	private:
		char*				m_data;
		PoolInfo			m_pools[APM_POOL_NUMBER];

		static constexpr size_t s_poolSize = sizeof(AlignedPool);
	};

	//-----------------------------------------------------------
	AlignedPoolManager& GetAlignedPoolManager();

	//-----------------------------------------------------------
	void setupPoolManager();

	//-----------------------------------------------------------
	template<typename T>
	class AlignedPoolAllocator
	{
	public:
		typedef T			value_type;
		
		typedef T*			pointer;
		typedef const T*	const_pointer;

		typedef T&			reference;
		typedef const T&	const_reference;

		typedef size_t		size_type;
		typedef int			difference_type;

		template <typename U>
		struct rebind
		{
			typedef AlignedPoolAllocator<U> other;
		};

	public:

		AlignedPoolAllocator() {};

		template <typename U>
		AlignedPoolAllocator(const AlignedPoolAllocator<U>& other) {};

		AlignedPoolAllocator& operator=(const AlignedPoolAllocator& other) {};

		
		static pointer address(reference r) { return &r; }
		static const_pointer address(const_reference r) { return &r; }
				
		static pointer allocate(const size_type n, const void* = 0)
		{
			pointer res = nullptr;
			if (n == 1)
			{
				res = static_cast<pointer>(GetAlignedPoolManager().malloc(sizeof(value_type)));
			}
			else if (n > 1)
			{
				res = static_cast<pointer>(GetAlignedPoolManager().malloc_n(sizeof(value_type), n));
			}

			if (!res)
			{
				throw std::bad_alloc();
			}

			return res;
		}

		static void deallocate(const_pointer ptr, const size_type blockNumber)
		{
			blockNumber == 1 
				? GetAlignedPoolManager().free(ptr) 
				: GetAlignedPoolManager().free_n(ptr, blockNumber);
		}

		static  void construct(pointer const p, value_type&& t)
		{
			new (static_cast<void*>(p)) value_type(std::forward<value_type>(t));
		}

		static  void destroy(const_pointer ptr)
		{
			ptr->~value_type();
		}
		
		bool operator==(const AlignedPoolAllocator&) const { return true; }
		bool operator!=(const AlignedPoolAllocator&) const { return false; }

		constexpr static size_type max_size() noexcept
		{
			return static_cast<size_type>(-1) / sizeof(value_type);
		}
	};
}// align_pool
#endif //ALIGNED_POOL_SRC_AP

