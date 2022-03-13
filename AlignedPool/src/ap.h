#ifndef ALIGNED_POOL_SRC_AP
#define ALIGNED_POOL_SRC_AP

#if defined(NDEBUG) || defined(DISABLE_LOG)
#define ALIGNED_POOL_ENABLE_MEM_LOG 0
#else
#define ALIGNED_POOL_ENABLE_MEM_LOG  1
#endif

namespace align_pool
{
	constexpr unsigned int KIBIBYTE = 1024u;
	constexpr unsigned int MEBIBYTE = KIBIBYTE * KIBIBYTE;

	struct AlignedPool
	{
	public:
		/*
		first parameter is blockSize
		second parameter is number of blocks of size blockSize
		*/
		explicit		AlignedPool(size_t blockSize, size_t blockCount);
						~AlignedPool();

		//delete copy constuctor and copy assigment
						AlignedPool(const AlignedPool& other) = delete;
		AlignedPool&	operator=(const AlignedPool& other) = delete;

		//custom move constructor and assignment
						AlignedPool(AlignedPool&& other) noexcept;
		AlignedPool&	operator=(AlignedPool&& other) noexcept;

		void*			malloc(size_t size);
		void			free(void* const ptr);
		
		inline bool		isFrom(const void* const ptr) const
														{
															return ptr >= m_data && ptr < static_cast<char*>(m_data) + m_blockSize * m_blockCount;
														}
	private:
		void			_init();
		void*			_getData(size_t idx)			const;
		size_t			_getNextFreeIdx(const size_t)	const;
		size_t			_tryMallocN(size_t n)			const;
		size_t			_findIdx(void* const p)			const;

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
	private:
		size_t			m_blockSize;
		size_t			m_blockCount;
		size_t			m_curFreeIdx;
		void*			m_data;
		size_t*			m_dataState;

	private:
		static constexpr size_t INVALID_ID = ~0u;
		static constexpr size_t s_minBlockSize = sizeof(void*);
	};

	static AlignedPool& GetInstance(const size_t size = 0, const void* const ptr = nullptr);
}// align_pool

//allocation function
void* operator new(const size_t size);
void* operator new[](const size_t size);

//non-allocating placement allocation functions
//void* operator new(const size_t size, void* ptr) noexcept;
//void* operator new[](const size_t size, void* ptr) noexcept;


void operator delete(void* block);
void operator delete[](void* block);
#endif //ALIGNED_POOL_SRC_AP

