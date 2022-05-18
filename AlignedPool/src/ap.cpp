#include "ap.h"

#include <memory>
#include <iostream>
#include <algorithm>

namespace align_pool
{
	//-----------------------------------------------------------
	AlignedPool::AlignedPool(size_t blockSize, size_t blockCount)
		:	
		m_blockSize{ blockSize },
		m_blockCount{ blockCount },
		m_curFreeIdx{ 0u },
		m_data{ nullptr },
		m_dataState{ nullptr }
	{
		_init();
	}

	//-----------------------------------------------------------
	AlignedPool::AlignedPool(size_t blockSize, size_t blockCount, char* ptr)
	{
		m_blockSize = blockSize;
		m_blockCount = blockCount;
		m_curFreeIdx = 0u;
		m_data = ptr;
		m_dataState = static_cast<size_t*>(static_cast<void*>(ptr + m_blockSize * m_blockCount));
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
	void* AlignedPool::malloc()
	{
		void* res = nullptr; 
		size_t idx = m_curFreeIdx;

		if (idx == INVALID_ID)
		{
			std::cout << "\nError in " << __FUNCTION__ << " there is no available memory for allocation of that number:[" << 1u << "] of memory blocks, each with size: [" << m_blockSize << "]\n";
		}
		else
		{
			res = static_cast<char*>(m_data) + idx * m_blockSize;
			*(m_dataState + idx) = 1u;
			m_curFreeIdx = _getNextFreeIdx(idx + 1u);
#if ALIGNED_POOL_ENABLE_MEM_LOG
			_log(idx, m_blockSize, MemHint::ALLOC);
#endif//ALIGNED_POOL_ENABLE_MEM_LOG
		}
		return res;
	}

	//-----------------------------------------------------------
	void* AlignedPool::malloc_n(const size_t blockNum)
	{
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
#endif//ALIGNED_POOL_ENABLE_MEM_LOG
		}
		return res;
	}

	//-----------------------------------------------------------
	void AlignedPool::free(const void* p)
	{
		size_t id = _findIdx(p);
		if (id == INVALID_ID)
		{
			std::cout << "\nError in " << __FUNCTION__ << " trying to free pointer:[0x" << p << "] which are not from that pool\n";
		}

#if ALIGNED_POOL_ENABLE_MEM_LOG
		size_t blockNum = *(m_dataState + id);
#endif//ALIGNED_POOL_ENABLE_MEM_LOG
		*(m_dataState + id) = 0;
		m_curFreeIdx = m_curFreeIdx < id ? m_curFreeIdx : id;
#if ALIGNED_POOL_ENABLE_MEM_LOG
		_log(id, m_blockSize * blockNum, MemHint::FREE);
#endif//ALIGNED_POOL_ENABLE_MEM_LOG
	}

	//-----------------------------------------------------------
	void AlignedPool::free_n(const void* p, size_t blockNumber)
	{
		size_t id = _findIdx(p);
		if (id == INVALID_ID)
		{
			std::cout << "\nError in " << __FUNCTION__ << " trying to free pointer:[0x" << p << "] which are not from that pool with block size[" << m_blockSize << "]\n";
		}

		if (id + blockNumber > m_blockCount)
		{
			std::cout << "\nError in " << __FUNCTION__ << " trying to free [" << blockNumber << "] elements , starting at address["
				<< p << "] which correspond to id[" << id << ", and goes out of range\n";
		}

		std::memset(m_dataState + id, 0, blockNumber);
		m_curFreeIdx = m_curFreeIdx < id ? m_curFreeIdx : id;
#if ALIGNED_POOL_ENABLE_MEM_LOG
		_log(id, m_blockSize * blockNumber, MemHint::FREE);
#endif//ALIGNED_POOL_ENABLE_MEM_LOG
	}

	//-----------------------------------------------------------
	void AlignedPool::_init()
	{
		if (m_data)
		{ 
			return;
		}

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
		size_t idx = _idx > m_curFreeIdx ? m_curFreeIdx : _idx;
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
		while (_n < n && idx + n <= m_blockCount)
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
	size_t AlignedPool::_findIdx(const void* p) const
	{
		size_t res = INVALID_ID;
		if (p >= m_data &&
			p < static_cast<char*>(m_data) + m_blockCount * m_blockSize)
		{
			res = (static_cast<const char*>(p) - static_cast<char*>(m_data)) / m_blockSize;
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
#endif//ALIGNED_POOL_ENABLE_MEM_LOG

#if APM_ENABLE_CACHING
	//-----------------------------------------------------------
	inline void AlignedPool::_fillRange(void*& b, void*& e) const
	{
		b = this->m_data;
		e = static_cast<char*>(this->m_data) + m_blockCount * m_blockSize;
	}
#endif //APM_ENABLE_CACHING
	
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

	//-----------------------------------------------------------
	AlignedPoolManager::PoolInfo::PoolInfo(AlignedPoolManager::PoolInfo&& other) noexcept
	{
		*this = std::move(other);
	}

	//-----------------------------------------------------------
	AlignedPoolManager::PoolInfo& AlignedPoolManager::PoolInfo::operator=(PoolInfo&& other) noexcept
	{
		this->blockSize = other.blockSize;
		other.blockSize = 0u;

		this->blockNumber = other.blockNumber;
		other.blockNumber = 0u;

		this->pool = other.pool;
		other.pool = nullptr;

		return *this;
	}

	//-----------------------------------------------------------
	AlignedPoolManager::AlignedPoolManager()
		: m_data{ nullptr }
	{
	}

	//-----------------------------------------------------------
	AlignedPoolManager::~AlignedPoolManager()
	{
		if (m_data)
			std::free(m_data);
	}

	//-----------------------------------------------------------
	AlignedPoolManager::AlignedPoolManager(AlignedPoolManager&& other) noexcept
	{
		*this = std::move(other);
	}

	//-----------------------------------------------------------
	AlignedPoolManager& AlignedPoolManager::operator=(AlignedPoolManager&& other) noexcept
	{
		this->m_data = other.m_data;
		other.m_data = nullptr;

		for (int i = 0; i < APM_POOL_NUMBER; i++)
		{
			this->m_pools[i] = std::move(other.m_pools[i]);
		}
		return *this;
	}

	//-----------------------------------------------------------
	void AlignedPoolManager::init()
	{
		if (m_data)
		{
			std::cout << "Error, AlignedPoolManager already initialized\n";
			return;
		}

		size_t totalSize = 0u;
		size_t offset = 0u;

		for (int i = 0; i < APM_POOL_NUMBER; i++)
		{
			if (m_pools[i].blockSize != 0)
			{
				//size of data + size of data states + size of AlignedePool itself
				totalSize += (m_pools[i].blockSize + sizeof(size_t)) * m_pools[i].blockNumber + s_poolSize;
			}
		}

		if (totalSize >= UINT_MAX)
		{
			std::cout << "Error during initialization. Too large memory block with size [" << totalSize << "]\n";
		}

		m_data = static_cast<char*>(std::malloc(totalSize));
		if (!m_data)
		{
			std::cout << "Error in function(" << __FUNCTION__ << "), memory is not allocated, please investigate\n";
		}

		std::memset(m_data, 0, totalSize);

		for (int i = 0; i < APM_POOL_NUMBER; i++)
		{
			if (m_pools[i].blockSize != 0)
			{

				m_pools[i].pool = static_cast<AlignedPool*>(static_cast<void*>(m_data + offset));
				offset += s_poolSize;

				new (m_pools[i].pool) AlignedPool(m_pools[i].blockSize, m_pools[i].blockNumber, m_data + offset);
#if ALIGNED_POOL_ENABLE_MEM_LOG
				std::cout << "Pool successfuly created in address [" << m_pools[i].pool << "]\n";
				std::cout << "Pool successfuly initialized with parameters blockSize[" << m_pools[i].blockSize << "]\t"
					<< "blockNumber[" << m_pools[i].blockNumber << "]\n"
					<< "address of the data  is[" << static_cast<void*>(m_data + offset) << "]\n"
					<< "address of the data states is[" << static_cast<void*>(m_data + offset + m_pools[i].blockSize * m_pools[i].blockNumber) << "]\n\n";
#endif//ALIGNED_POOL_ENABLE_MEM_LOG
				offset += (m_pools[i].blockSize + sizeof(size_t)) * m_pools[i].blockNumber;
			}
		}

		std::sort(m_pools, m_pools + APM_POOL_NUMBER, [](const PoolInfo& l, const PoolInfo& r) ->bool {
			if (l.blockSize == 0) return false;
			if (r.blockSize == 0) return true;
			return l.blockSize < r.blockSize; });
	}

	//-----------------------------------------------------------
	void AlignedPoolManager::addPool(size_t blockSize, size_t blockNum)
	{
		for (int i = 0; i < APM_POOL_NUMBER; i++)
		{
			if (m_pools[i].blockSize == 0 && !m_pools[i].pool)
			{
				m_pools[i].blockSize = blockSize;
				m_pools[i].blockNumber = blockNum;
				return;
			}
		}
	}

	//-----------------------------------------------------------
	void AlignedPoolManager::removePool(size_t blockSize, size_t blockNum)
	{
		for (int i = 0; i < APM_POOL_NUMBER; i++)
		{
			if (m_pools[i].blockSize == blockSize && !m_pools[i].pool)
			{
				m_pools[i].blockSize = 0;
				m_pools[i].blockNumber = 0;
				return;
			}
		}
	}

	//-----------------------------------------------------------
	void* AlignedPoolManager::malloc(size_t size)
	{
#if APM_ENABLE_CACHING
		if (m_cacheMalloc.size != size)
		{
			m_cacheMalloc.hits = 0u; 
			m_cacheMalloc.id = ~0u;
#if ALIGNED_POOL_ENABLE_MEM_LOG
			std::cout << "malloc(): cache miss of the object size[" << size << "], hit count[" << m_cacheMalloc.hits << "]\n";
			std::cout << "size in cache[" << m_cacheMalloc.size << "]\n";
#endif//ALIGNED_POOL_ENABLE_MEM_LOG
		}
		else if (m_cacheMalloc.hits > APM_HIT_COUNT_TO_BE_CACHED)
		{
#if ALIGNED_POOL_ENABLE_MEM_LOG
			std::cout << "malloc(): cache hit of the object size[" << size << "]\n";
			std::cout << "memory is allocated from pool[" << m_pools[m_cacheMalloc.id].pool << "], with blockSize[" << m_pools[m_cacheMalloc.id].blockSize <<"]\n";
#endif//ALIGNED_POOL_ENABLE_MEM_LOG
			return m_pools[m_cacheMalloc.id].pool->malloc();
		}
#endif//APM_ENABLE_CACHING
		for (int i = 0; i < APM_POOL_NUMBER; i++)
		{
			if (m_pools[i].blockSize >= size)
			{
#if APM_ENABLE_CACHING
				m_cacheMalloc.hits++;
				m_cacheMalloc.id = i;
				m_cacheMalloc.size = size;
#endif//APM_ENABLE_CACHING
#if ALIGNED_POOL_ENABLE_MEM_LOG
				std::cout << "malloc(): allocate object with size[" << size << "]\n";
				std::cout << "memory is allocated from pool[" << m_pools[m_cacheMalloc.id].pool << "], with blockSize[" << m_pools[m_cacheMalloc.id].blockSize << "]\n";
#endif//ALIGNED_POOL_ENABLE_MEM_LOG
				return m_pools[i].pool->malloc();
			}
		}
		std::cout << "There is no pool with blockSize[" << size << "]\n";
		return nullptr;
	}

	//-----------------------------------------------------------
	void* AlignedPoolManager::malloc_n(size_t size, size_t blockNumber)
	{
#if APM_ENABLE_CACHING
		if (m_cacheMalloc.size != size)
		{
			m_cacheMalloc.hits = 0u;
			m_cacheMalloc.id = ~0u;
#if ALIGNED_POOL_ENABLE_MEM_LOG
			std::cout << "malloc_n(): cache miss of the object size[" << size << "]\n";
			std::cout << "size in cache[" << m_cacheMalloc.size << "]\n";
#endif//ALIGNED_POOL_ENABLE_MEM_LOG
		}
		else if (m_cacheMalloc.hits > APM_HIT_COUNT_TO_BE_CACHED)
		{
#if ALIGNED_POOL_ENABLE_MEM_LOG
			std::cout << "malloc_n(): cache hit of the object size[" << size << "], hit count[" << m_cacheMalloc.hits << "]\n";
			std::cout << "memory is allocated from pool[" << m_pools[m_cacheMalloc.id].pool << "], with blockSize[" << m_pools[m_cacheMalloc.id].blockSize << "]\n";
#endif//ALIGNED_POOL_ENABLE_MEM_LOG
			return m_pools[m_cacheMalloc.id].pool->malloc_n(blockNumber);
		}
#endif//APM_ENABLE_CACHING
		for (int i = 0; i < APM_POOL_NUMBER; i++)
		{
			if (m_pools[i].blockSize >= size)
			{
#if APM_ENABLE_CACHING
				m_cacheMalloc.hits++;
				m_cacheMalloc.id = i;
				m_cacheMalloc.size = size;
#endif//APM_ENABLE_CACHING
#if ALIGNED_POOL_ENABLE_MEM_LOG
				std::cout << "malloc_n(): allocate object with size[" << size << "]\n";
				std::cout << "memory is allocated from pool[" << m_pools[m_cacheMalloc.id].pool << "], with blockSize[" << m_pools[m_cacheMalloc.id].blockSize << "]\n";
#endif//ALIGNED_POOL_ENABLE_MEM_LOG
				return m_pools[i].pool->malloc_n(blockNumber);
			}
		}
		std::cout << "There is no pool with blockSize[" << size << "], maximal block size in pool list is[" << m_pools[APM_POOL_NUMBER - 1].blockSize << "]\n";
		return nullptr;
	}

	//-----------------------------------------------------------
	void AlignedPoolManager::free(const void* ptr)
	{
#if APM_ENABLE_CACHING
		if (m_cacheFree.begin > ptr || m_cacheFree.end < ptr)
		{
			m_cacheFree.hits = 0u;
			m_cacheFree.id = ~0u;
#if ALIGNED_POOL_ENABLE_MEM_LOG
			std::cout << "free(): cache miss of the object with address[" << ptr << "] in range[" << m_cacheFree.begin << ", " << m_cacheFree.end << ")\n";
			std::cout << "size in cache[" << m_cacheFree.size << "]\n";
#endif//ALIGNED_POOL_ENABLE_MEM_LOG
		}
		else if (m_cacheFree.hits >= APM_HIT_COUNT_TO_BE_CACHED)
		{
			m_pools[m_cacheFree.id].pool->free(ptr);
#if ALIGNED_POOL_ENABLE_MEM_LOG
			std::cout << "free(): cache hit of the object with address[" << ptr << "] in range[" << m_cacheFree.begin << ", " << m_cacheFree.end << ")\n";
			std::cout << "size in cache[" << m_cacheFree.size << "], hit count[" << m_cacheFree.hits << "\n";
			std::cout << "pool[" << m_pools[m_cacheFree.id].pool << "], with block size[" << m_pools[m_cacheFree.id].blockSize << "\n";
#endif//ALIGNED_POOL_ENABLE_MEM_LOG
			return;
		}
#endif//APM_ENABLE_CACHING
		for (int i = 0; i < APM_POOL_NUMBER; i++)
		{
			if (m_pools[i].pool && m_pools[i].pool->isFrom(ptr))
			{
				m_pools[i].pool->free(ptr);
#if APM_ENABLE_CACHING
				m_cacheFree.hits++;
				m_cacheFree.id = i;
				if (m_cacheFree.hits == 1u)
					m_pools[i].pool->_fillRange(m_cacheFree.begin, m_cacheFree.end);
#endif//APM_ENABLE_CACHING
#if ALIGNED_POOL_ENABLE_MEM_LOG
				std::cout << "free(): memory address[" << ptr << "] freed from pool within  range[" << m_cacheFree.begin << ", " << m_cacheFree.end << ")\n";
				std::cout << "size in cache[" << m_cacheFree.size << "]\n";
				std::cout << "pool[" << m_pools[m_cacheFree.id].pool << "], with block size[" << m_pools[m_cacheFree.id].pool->m_blockSize << "\n";
#endif//ALIGNED_POOL_ENABLE_MEM_LOG
				return;
			}
		}
		std::cout << "Object with address[" << ptr << "] doesn't reside in any pool\n";
	}

	//-----------------------------------------------------------
	void AlignedPoolManager::free_n(const void* ptr, size_t blockNumber)
	{
#if APM_ENABLE_CACHING
		if (m_cacheFree.begin > ptr || m_cacheFree.end < ptr)
		{
			m_cacheFree.hits = 0u;
			m_cacheFree.id = ~0u;
#if ALIGNED_POOL_ENABLE_MEM_LOG
			std::cout << "free(): cache miss of the object with address[" << ptr << "] in range[" << m_cacheFree.begin << ", " << m_cacheFree.end << ")\n";
			std::cout << "size in cache[" << m_cacheFree.size << "]\n";
#endif//ALIGNED_POOL_ENABLE_MEM_LOG
		}
		else if (m_cacheFree.hits >= APM_HIT_COUNT_TO_BE_CACHED)
		{
			m_pools[m_cacheFree.id].pool->free_n(ptr, blockNumber);
#if ALIGNED_POOL_ENABLE_MEM_LOG
			std::cout << "free(): cache hit of the object with address[" << ptr << "] in range[" << m_cacheFree.begin << ", " << m_cacheFree.end << ")\n";
			std::cout << "size in cache[" << m_cacheFree.size << "], hit count[" << m_cacheFree.hits << "\n";
			std::cout << "pool[" << m_pools[m_cacheFree.id].pool << "], with block size[" << m_pools[m_cacheFree.id].blockSize << "\n";
#endif//ALIGNED_POOL_ENABLE_MEM_LOG
			return;
		}
#endif//APM_ENABLE_CACHING
		for (int i = 0; i < APM_POOL_NUMBER; i++)
		{
			if (m_pools[i].pool && m_pools[i].pool->isFrom(ptr))
			{
				m_pools[i].pool->free_n(ptr, blockNumber);
#if APM_ENABLE_CACHING
				m_cacheFree.hits++;
				m_cacheFree.id = i;
				if (m_cacheFree.hits == 1u)
					m_pools[i].pool->_fillRange(m_cacheFree.begin, m_cacheFree.end);
#endif//APM_ENABLE_CACHING
#if ALIGNED_POOL_ENABLE_MEM_LOG
				std::cout << "free(): memory address[" << ptr << "] freed from pool within  range[" << m_cacheFree.begin << ", " << m_cacheFree.end << ")\n";
				std::cout << "size in cache[" << m_cacheFree.size << "]\n";
				std::cout << "pool[" << m_pools[m_cacheFree.id].pool << "], with block size[" << m_pools[m_cacheFree.id].pool->m_blockSize << "\n";
#endif//ALIGNED_POOL_ENABLE_MEM_LOG
				return;
			}
		}
		std::cout << "Object with address[" << ptr << "] doesn't reside in any pool\n";
	}

	//-----------------------------------------------------------
	AlignedPoolManager g_poolManager;

	//-----------------------------------------------------------
	void setupPoolManager()
	{
		g_poolManager.addPool(4, 35 * 1000);
		g_poolManager.addPool(8, 35 * 1000);
		g_poolManager.addPool(16, 35 * 1000);
		g_poolManager.addPool(64, 35 * 1000);
		g_poolManager.addPool(256, 35 * 1000);
		g_poolManager.addPool(512, 35 * 1000);

		g_poolManager.init();
	}
}//align_pool