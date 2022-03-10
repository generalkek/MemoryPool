#include "ap.h"

#include <memory>
#include <iostream>

namespace align_pool
{
	//-----------------------------------------------------------
	AlignedPool::AlignedPool(size_t blockSize, size_t blockCount)
		:	
		m_blockCount{ blockCount },
		m_data{nullptr},
		m_dataState{nullptr}
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

		return *this;
	}

	//-----------------------------------------------------------
	void* AlignedPool::malloc(size_t size)
	{
		size_t blockNum = size / m_blockSize + static_cast<bool>(size % m_blockSize);
		void* res = _tryMallocN(blockNum);
		size_t idx = INVALID_ID;

		if (!res)
		{
			std::cout << "\nError in " << __FUNCTION__ << " pool is full, no available memory for allocation of that number:[" << blockNum << "] of memory blocks\n";
		}
		else
		{
			idx = _findIdx(res);
			*(m_dataState + idx) = blockNum;
#if ALIGNED_POOL_ENABLE_MEM_LOG
			_log(idx, m_blockSize * blockNum, MemHint::ALLOC);
#endif
		}

		return res;
	}

	//-----------------------------------------------------------
	void AlignedPool::free(void* const p)
	{
		int id = _findIdx(p);
		if (id == INVALID_ID)
		{
			std::cout << "\nError in " << __FUNCTION__ << " trying to free pointer:[0x" << p << "] which are not from that pool\n";
		}
		
		size_t blockNum = *(m_dataState + id);
		*(m_dataState + id) = 0;
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
	void* AlignedPool::_getData(size_t idx) const
	{
		void* res = nullptr;
		if (idx < m_blockCount)
			res = static_cast<char*>(m_data) + idx * m_blockSize;
		return res;
	}

	//-----------------------------------------------------------
	size_t AlignedPool::_getCurIdx() const
	{
		size_t idx = 0;
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
	void* AlignedPool::_tryMallocN(size_t n) const
	{
		void* res = nullptr;
		size_t idx = _getCurIdx();
		size_t _n = 0;
		while (_n < n && idx < m_blockCount)
		{
			if (*(m_dataState + _n + idx) != 0)
			{
				idx += _n;
				_n = 0;
			}
			_n++;
		}

		if (_n == n)
		{
			res = _getData(idx);
		}
		
		return res;
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
		std::cout << "Max block count:[" << m_blockCount << "]\n";
		std::cout << "Percent of free blocks:[" << 100.0f * (1.0f - (static_cast<float>(curSize) / m_blockCount))<< "%]\n";
		std::cout << "#--------------------------------------------------------------------#\n\n";
	}
#endif

	//-----------------------------------------------------------
	AlignedPool& GetInstance()
	{
		static AlignedPool smallPool(400, 2048);
		/*static AlignedPool mediumPool(32, 1000);
		static AlignedPool largePool(128, 1000);

		if (size <= 4)
		{
			return smallPool;
		}
		else if (size <= 32)
		{
			return mediumPool;
		}
		else
		{
			return largePool;
		}*/
		return smallPool;
	}
	//ALIGNED_POOL_ENABLE_MEM_LOG
}//align_pool

//global operators overload
//allocation function
//-----------------------------------------------------------
void* operator new(const size_t size)
{
	return align_pool::GetInstance().malloc(size);
}

//-----------------------------------------------------------
void* operator new[](const size_t size)
{
	return align_pool::GetInstance().malloc(size);
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
	align_pool::GetInstance().free(block);
}

//-----------------------------------------------------------
void operator delete[](void* block)
{
	align_pool::GetInstance().free(block);
}