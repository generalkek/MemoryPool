#include "sbp.h"
#include <memory>
#include <iostream>

namespace sbp
{
	//-----------------------------------------------------------
	StackBasedPool::StackBasedPool()
		:
		m_stack{nullptr},
		m_stackSize{0},
		m_curSize{0}
	{
		_init(MEBIBYTE);
	}

	//-----------------------------------------------------------
	StackBasedPool::StackBasedPool(const size_t size)
		:
		m_stack{nullptr},
		m_stackSize{0},
		m_curSize{0}
	{
		_init(size);
	}

	//-----------------------------------------------------------
	StackBasedPool::StackBasedPool(StackBasedPool&& p)
	{
		this->operator=(std::move(p));
	}

	//-----------------------------------------------------------
	StackBasedPool::~StackBasedPool()
	{
		if (m_stack)
		{
			m_stack = static_cast<char*>(m_stack) - m_curSize;
			std::free(m_stack);
		}
	}

	//-----------------------------------------------------------
	StackBasedPool& StackBasedPool::operator=(StackBasedPool&& p) noexcept
	{
		this->m_stack = p.m_stack;
		p.m_stack = nullptr;

		this->m_stackSize = p.m_stackSize;
		this->m_curSize = p.m_curSize;

		p.~StackBasedPool();
		return *this;
	}
	//-----------------------------------------------------------
	void* StackBasedPool::malloc(const size_t size)
	{
		void* ptr = nullptr;
		if (m_stackSize > m_curSize + size + sizeof(void*))
		{
			//get result ptr
			ptr = static_cast<char*>(m_stack) + sizeof(void*);

			//get new position for free block
			m_stack = (static_cast<char*>(ptr) + size);

			//wtire in the size of new block
			*(static_cast<int*>(m_stack)) = size;

			m_curSize = m_curSize + size + sizeof(void*);

#if STACK_BASED_POOL_ENABLE_MEM_LOG
			_log(ptr, size, MemHint::ALLOC);
#endif//STACK_BASED_POOL_ENABLE_MEM_LOG

		}
		else
		{
			std::cout << "Not enough memory for allocation of size [" << size << "]\n"
				<< "Currently available amount of memory is [" << m_stackSize - m_curSize << "]\n";
		}
		return ptr;
	} 

	//-----------------------------------------------------------
	void StackBasedPool::free()
	{
		//retrive size of prev block
		size_t ps = *(static_cast<int*>(m_stack));

#if STACK_BASED_POOL_ENABLE_MEM_LOG 
		//for logging purposes
		void* ptr = m_stack;
#endif//STACK_BASED_POOL_ENABLE_MEM_LOG

		//move back free block 
		m_stack = static_cast<char*>(m_stack) - ps - sizeof(void*);
		m_curSize = m_curSize - ps - sizeof(void*);

#if STACK_BASED_POOL_ENABLE_MEM_LOG
		_log(ptr, ps, MemHint::FREE);
#endif//STACK_BASED_POOL_ENABLE_MEM_LOG
	}

	//-----------------------------------------------------------
	size_t StackBasedPool::getPrevBlockSize() const
	{
		return m_stack ?
			*(static_cast<int*>(m_stack)) : 0;
	}

	//-----------------------------------------------------------
	void StackBasedPool::_init(const size_t size)
	{
		size_t s = size;
		if (s <= 0)
		{
			std::cout << "Size supplied for initialization of stack is less or equal to zero.\nThus stack was initialized with default value of 1 mebibyte.\n";
			s = MEBIBYTE;
		}

		if (!m_stack)
		{
			m_stack = std::malloc(s);
			m_stackSize = s;
			if (m_stack)
			{
				*(static_cast<int*>(m_stack)) = 0;
			}
		}
	}
#if STACK_BASED_POOL_ENABLE_MEM_LOG
	//-----------------------------------------------------------
	void StackBasedPool::_log(const void* const p, const size_t s, const MemHint h) const
	{
		std::cout << "#--------------------------------------------------------------------#\n";
		if (h != MemHint::INFO)
		{
			if (h == MemHint::ALLOC)
			{
				std::cout << "At memory location [" << p << "] has been allocated [" << s << "] bytes of memory\n";
			}
			else if (h == MemHint::FREE)
			{
				std::cout << "At memory location [" << p << "] has been freed [" << s << "] bytes of memory\n";
			}

			std::cout << "New available amount of memory is [" << m_stackSize - m_curSize << "]\n"
				<< "New addres of free memory block is [" << m_stack << "]\n";
		}

		std::cout << "Current occupied size [" << m_curSize << "]\n" << "Total size: [" << m_stackSize << "]\n";
		std::cout << "Memory occupied [" << static_cast<float>(m_curSize) / static_cast<float>(m_stackSize) * 100.0f << "%]\n";
		std::cout << "#--------------------------------------------------------------------#\n\n";
	}
#endif//STACK_BASED_POOL_ENABLE_MEM_LOG

	//-----------------------------------------------------------
	StackBasedPool& GetInstance(const size_t s)
	{
		static StackBasedPool stack{ s };
		return stack;
	}
}//namespcae sbp

	void* operator new(const size_t size)
	{
		return sbp::GetInstance().malloc(size);
	}