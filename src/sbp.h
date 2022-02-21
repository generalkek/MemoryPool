#ifndef MP_SRC_STACKBASEDPOOL
#define MP_SRC_STACKBASEDPOOL

#if defined(NDEBUG) || defined(DISABLE_LOG)
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
		explicit				StackBasedPool();
		explicit				StackBasedPool(const size_t size);

								~StackBasedPool();
		//remove copy constuctor and copy assigment
								StackBasedPool(const StackBasedPool& pool) = delete;
		StackBasedPool&			operator=(const StackBasedPool& pool) = delete;

		//define non-trivial	 move constructor and move assigment
								StackBasedPool(StackBasedPool&& p) noexcept;//maybe make explicit
		StackBasedPool&			operator=(StackBasedPool&& pool) noexcept;

		//main functions
		void*					malloc(const size_t size);
		void					free(void* ptr);

		inline bool				isEmpty() const {
													return m_curSize > 0;
												};

	private:
		void					_init(const size_t size);
		inline static void*&	_getPrev(void* p) 
											{
												return *(static_cast<void**>(p));
											}
#if STACK_BASED_POOL_ENABLE_MEM_LOG
	private:
		enum class MemHint
		{
			INFO = 0,
			ALLOC = 1 << INFO,
			FREE = 1 << ALLOC,
		};

		void					_log(const void* const p, const size_t s, const MemHint h = MemHint::INFO) const;
#endif//STACK_BASED_POOL_ENABLE_MEM_LOG

	private:
		void*					m_stack;//pointer to all memory
		unsigned long long		m_stackSize;//max size 
		unsigned long long		m_curSize;

		//implementation detail, thus make it private
		static constexpr size_t ptrSize = sizeof(void*);
	};

	static StackBasedPool& GetInstance(const size_t s = MEBIBYTE);
}//namespcae sbp


//allocation function
void* operator new(const size_t size);
void* operator new[](const size_t size);

//non-allocating placement allocation functions
//void* operator new(const size_t size, void* ptr) noexcept;
//void* operator new[](const size_t size, void* ptr) noexcept;


void operator delete(void* block);
void operator delete[](void* block);
#endif //MP_SRC_STACKBASEDPOOL
