#ifndef MP_SRC_STACKBASEDPOOL
#define MP_SRC_STACKBASEDPOOL

#if defined(NDEBUG)
#define STACK_BASED_POOL_ENABLE_MEM_LOG 0
#else 
#define STACK_BASED_POOL_ENABLE_MEM_LOG 1
#endif

#pragma warning(disable:26495)

namespace sbp
{
	constexpr unsigned int KIBIBYTE = 1024;//initial stack size;
	constexpr unsigned int MEBIBYTE = KIBIBYTE * KIBIBYTE;//initial stack size;

	struct StackBasedPool
	{
		explicit			StackBasedPool();
		explicit			StackBasedPool(const size_t size);
							StackBasedPool(StackBasedPool&& p);
							~StackBasedPool();
		
							StackBasedPool(const StackBasedPool& pool) = delete;
		StackBasedPool&		operator=(const StackBasedPool& pool) = delete;
		StackBasedPool&		operator=(StackBasedPool&& pool) noexcept;


		void*				malloc(const size_t size);
		void				free();

		size_t				getPrevBlockSize() const;
		private:
		void				_init(const size_t size);
	
#if STACK_BASED_POOL_ENABLE_MEM_LOG
	private:
		enum class MemHint
		{
			INFO = 0,
			ALLOC = 1 << INFO,
			FREE = 1 << ALLOC,
		};
		void				_log(const void* const p, const size_t s, const MemHint h = MemHint::INFO) const;
#endif//STACK_BASED_POOL_ENABLE_MEM_LOG

	private:
		void*				m_stack;//pointer to all memory
		unsigned long		m_stackSize;
		unsigned long		m_curSize;
	};

	static StackBasedPool& GetInstance(const size_t s = MEBIBYTE);
}//namespcae sbp

	void* operator new(const size_t size);
#endif //MP_SRC_STACKBASEDPOOL
