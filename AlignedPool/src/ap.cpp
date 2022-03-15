#include "ap.h"

#include <memory>
#include <iostream>

namespace align_pool
{
	//-----------------------------------------------------------
	AlignedPool::AlignedPool(size_t blockSize, size_t blockCount)
		:	
		m_blockCount{ blockCount },
		m_curFreeIdx{ 0u },
		m_data{ nullptr },
		m_dataState{ nullptr }
	{
		m_blockSize = blockSize > s_minBlockSize ? blockSize : s_minBlockSize;
		_init();
	}

	//-----------------------------------------------------------
	AlignedPool::~AlignedPool()
	{
		if (m_data)
			std::free(m_data);
		if (m_dataState)
			std::free(m_dataState);
	}

#pragma warning(push)
#pragma warning(disable:26495)//disable warning about non-initialized members
//-----------------------------------------------------------
	AlignedPool::AlignedPool(AlignedPool&& other) noexcept
	{
		this->operator=(std::move(other));
	}
#pragma warning(pop)

	//-----------------------------------------------------------
	AlignedPool& AlignedPool::operator=(AlignedPool&& other) noexcept
	{
		this->m_data = other.m_data;
		other.m_data = nullptr;

		this->m_dataState = other.m_dataState;
		other.m_dataState = nullptr;

		this->m_blockSize = other.m_blockSize;
		other.m_blockSize = 0;

		this->m_blockCount = other.m_blockCount;
		other.m_blockCount = 0;

		this->m_curFreeIdx = other.m_curFreeIdx;
		other.m_curFreeIdx = 0;

		return *this;
	}

	//-----------------------------------------------------------
	void* AlignedPool::malloc(size_t size)
	{
		size_t blockNum = size / m_blockSize + static_cast<bool>(size % m_blockSize);
		void* res = nullptr; 
		size_t idx = _tryMallocN(blockNum);

		if (idx == INVALID_ID)
		{
			std::cout << "\nError in " << __FUNCTION__ << " there is no available memory for allocation of that number:[" << blockNum << "] of memory blocks, each with size: [" << m_blockSize << "]\n";
		}
		else
		{
			res = static_cast<char*>(m_data) + idx * m_blockSize;
			*(m_dataState + idx) = blockNum;
			m_curFreeIdx = _getNextFreeIdx(idx + blockNum);
#if ALIGNED_POOL_ENABLE_MEM_LOG
			_log(idx, m_blockSize * blockNum, MemHint::ALLOC);
#endif
		}

		return res;
	}

	//-----------------------------------------------------------
	void AlignedPool::free(void* const p)
	{
		size_t id = _findIdx(p);
		if (id == INVALID_ID)
		{
			std::cout << "\nError in " << __FUNCTION__ << " trying to free pointer:[0x" << p << "] which are not from that pool\n";
		}

#if ALIGNED_POOL_ENABLE_MEM_LOG
		size_t blockNum = *(m_dataState + id);
#endif
		*(m_dataState + id) = 0;
		m_curFreeIdx = m_curFreeIdx < id ? m_curFreeIdx : id;
#if ALIGNED_POOL_ENABLE_MEM_LOG
		_log(id, m_blockSize * blockNum, MemHint::FREE);
#endif
	}

	//-----------------------------------------------------------
	void AlignedPool::_init()
	{
		if (m_data)
		{ 
			return;
		}

		//check whether block size and block count are correct
		m_data = std::malloc(m_blockSize * m_blockCount);
		m_dataState = static_cast<size_t*>(std::malloc(m_blockCount * sizeof(size_t)));

		if (m_dataState)
		{
			std::memset(m_dataState, 0, m_blockCount * sizeof(size_t));
		}
	}

	//-----------------------------------------------------------
	inline void* AlignedPool::_getData(const size_t idx) const
	{
		if (idx < m_blockCount)
			return static_cast<char*>(m_data) + idx * m_blockSize;
		return nullptr;
	}

	//-----------------------------------------------------------
	size_t AlignedPool::_getNextFreeIdx(const size_t _idx) const
	{
		size_t idx = _idx;
		while (*(m_dataState + idx) != 0 && idx < m_blockCount)
		{
			idx += *(m_dataState + idx);
		}
		if (idx >= m_blockCount)
		{
			idx = INVALID_ID;
		}
		return idx;
	}

	//-----------------------------------------------------------
	size_t AlignedPool::_tryMallocN(size_t n) const
	{
		size_t idx = m_curFreeIdx;
		size_t _n = 1;
		while (_n < n && idx < m_blockCount)
		{
			if (*(m_dataState + _n + idx) != 0)
			{
				idx += _n + *(m_dataState + _n + idx);
				_n = 1;
			}
			_n++;
		}

		if (_n == n)
		{
			return idx;
		}
		
		return INVALID_ID;
	}

	//-----------------------------------------------------------
	size_t AlignedPool::_findIdx(void* const p) const
	{
		size_t res = INVALID_ID;
		if (p >= m_data &&
			p < static_cast<char*>(m_data) + m_blockCount * m_blockSize)
		{
			res = (static_cast<char*>(p) - static_cast<char*>(m_data)) / m_blockSize;
		}
		return res;
	}

#if ALIGNED_POOL_ENABLE_MEM_LOG
	//-----------------------------------------------------------
	void AlignedPool::_log(int blockId, size_t memory, MemHint hint) const
	{
		std::cout << "#--------------------------------------------------------------------#\n";
		if (hint == MemHint::ALLOC)
		{
			std::cout << "At memory location:[0x" << _getData(blockId) << "] allocated " << memory << " bytes of memory\n";
		}
		else if (hint == MemHint::FREE)
		{
			std::cout << "At memory location:[0x" << _getData(blockId) << "] freed " << memory << " bytes of memory\n";
		}
		std::cout << "Block index:[" << blockId << "]\n";

		size_t curSize = 0;
		
		for (size_t i = 0, idx = 0; i < m_blockCount;)
		{
			idx = *(m_dataState + i);
			if (idx != 0)
			{
				curSize += idx;
				i += idx;
			}
			else
			{
				i++;
			}
		}
		std::cout << "Current occupied blocks:[" << curSize << "]\n";
		std::cout << "Current free ID :[" << m_curFreeIdx << "]\n";
		std::cout << "Max block count:[" << m_blockCount << "]\n";
		std::cout << "Percent of free blocks:[" << 100.0f * (1.0f - (static_cast<float>(curSize) / m_blockCount))<< "%]\n";
		std::cout << "#--------------------------------------------------------------------#\n\n";
	}
#endif

	//-----------------------------------------------------------
	AlignedPool& GetInstance(const size_t size /*0*/, const void* const ptr /*nullptr*/)
	{
		static AlignedPool smallPool(4, 1000 * 20);
		static AlignedPool mediumPool(32, 1000 * 20);
		static AlignedPool largePool(128, 1000 * 20);
		static AlignedPool giantPool(512, 1000 * 20);

		if (size == 0 && ptr)
		{
			if (mediumPool.isFrom(ptr))
				return mediumPool;
			else if (largePool.isFrom(ptr))
				return largePool;
			else if (smallPool.isFrom(ptr))
				return smallPool;
			else if (giantPool.isFrom(ptr))
				return giantPool;
			else
			{
				throw std::bad_exception();
			}
		}
		else
		{
			if (size <= 4)
			{
				return smallPool;
			}
			else if (size <= 32)
			{
				return mediumPool;
			}
			else if (size <= 128)
			{
				return largePool;
			}
			else
			{
				return giantPool;
			}
		}
	}
	//ALIGNED_POOL_ENABLE_MEM_LOG
}//align_pool

//global operators overload
//allocation function
//-----------------------------------------------------------
void* operator new(const size_t size)
{
	return align_pool::GetInstance(size).malloc(size);
}

//-----------------------------------------------------------
void* operator new[](const size_t size)
{
	return align_pool::GetInstance(size).malloc(size);
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
	align_pool::GetInstance(0, block).free(block);
}

//-----------------------------------------------------------
void operator delete[](void* block)
{
	align_pool::GetInstance(0, block).free(block);
}