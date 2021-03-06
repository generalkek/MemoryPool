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
	StackBasedPool::StackBasedPool(StackBasedPool&& p) noexcept
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
		
		p.m_stackSize = p.m_curSize = 0;

		return *this;
	}

	//-----------------------------------------------------------
	void* StackBasedPool::malloc(const size_t size)
	{
		void* ptr = nullptr;

		if (m_stackSize > m_curSize + size + ptrSize)
		{
			//get result ptr
			ptr = m_stack;

			//get new position for metadata
			m_stack = (static_cast<char*>(ptr) + size);

			//wtire in the addres of allocated block
			_getPrev(m_stack) = ptr;

			//move m_stack to new free position
			m_stack = static_cast<char*>(m_stack) + ptrSize;
			
			m_curSize = m_curSize + size + ptrSize;

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
	void StackBasedPool::free(void* ptr)
	{
		if (m_curSize <= 0)
		{
			std::cout << "Trying to free from empty stack!\n";
			return;
		}
		
		//move back to pointer with addres of block that we want to free
		void* fPtr = static_cast<char*>(m_stack) - ptrSize;

		if (_getPrev(fPtr) != ptr)
		{
			std::cout << "Error trying to free: [0x" << ptr << "] in wrong order. Memory is not freed.\n";
			return;
		}
		//move back free block 
		m_stack = _getPrev(fPtr);

		//calculate size of freed block
		size_t ps = static_cast<char*>(fPtr) - static_cast<char*>(m_stack);

		m_curSize = m_curSize - ps - ptrSize;

#if STACK_BASED_POOL_ENABLE_MEM_LOG
		_log(static_cast<char*>(fPtr) + ptrSize, ps, MemHint::FREE);
#endif//STACK_BASED_POOL_ENABLE_MEM_LOG
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
				std::cout << "At memory location [0x" << p << "] has been allocated [" << s << "] bytes of memory\n";
			}
			else if (h == MemHint::FREE)
			{
				std::cout << "At memory location [0x" << p << "] has been freed [" << s << "] bytes of memory\n";
			}

			std::cout << "New available amount of memory is [" << m_stackSize - m_curSize << "]\n"
				<< "New addres of free memory block is [0x" << m_stack << "]\n";
		}

		std::cout << "Current occupied size: [" << m_curSize << "]\n" << "Total size: [" << m_stackSize << "]\n";
		std::cout << "Memory occupied: [" << static_cast<float>(m_curSize) / static_cast<float>(m_stackSize) * 100.0f << "%]\n";
		std::cout << "#--------------------------------------------------------------------#\n\n";
	}
#endif//STACK_BASED_POOL_ENABLE_MEM_LOG

	//-----------------------------------------------------------
	StackBasedPool& GetInstance(const size_t s)
	{
		static StackBasedPool stack{ s > MEBIBYTE ? s : MEBIBYTE };
		return stack;
	}
}//namespcae sbp

//global operators overload
//allocation function
//-----------------------------------------------------------
void* operator new(const size_t size)
{
	return sbp::GetInstance(size).malloc(size);
}

//-----------------------------------------------------------
void* operator new[](const size_t size)
{
	return sbp::GetInstance(size).malloc(size);
}

//non-allocating placement allocation functions
////-----------------------------------------------------------
//void* operator new(const size_t size, void* ptr) noexcept
//{
//	(void)size;
//	return ptr;
//}
//
////-----------------------------------------------------------
//void* operator new[](const size_t size, void* ptr) noexcept
//{
//	(void)size;
//	return ptr;
//}

//-----------------------------------------------------------
void operator delete(void* block)
{
	sbp::GetInstance().free(block);
}

//-----------------------------------------------------------
void operator delete[](void* block)
{
	sbp::GetInstance().free(block);
}
