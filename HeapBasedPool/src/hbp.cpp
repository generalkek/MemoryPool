#include "hbp.h"

namespace hbp
{

	void FreeListStorage::addBLock(void* ptr, size_t size)
	{
		void* pos = _findPrev(ptr);
		if (!pos)
			_makeList(ptr, m_data, size);
		else
			_nextOf(pos) = _makeList(ptr, _nextOf(pos), size);
	}

	void* FreeListStorage::_findPrev(void* const ptr) const
	{
		if (!m_data || ptr < m_data)
			return nullptr;
		
		void* iter = _nextOf(m_data);

		while (true)
		{
			if (!_nextOf(iter) || ptr > iter)
				return iter;
			iter = _nextOf(iter);
		}
	}

	void* FreeListStorage::_makeList(void* const begin, void* const end, size_t size)
	{
		char* old = static_cast<char*>(begin) + size - sizeof(void*);

		_nextOf(old) = end;

		if (begin == old)
			return begin;

		for (char* iter = old - sizeof(void*); iter != begin; old = iter, iter-= sizeof(void*))
		{
			_nextOf(iter) = old;
		}

		_nextOf(begin) = old;

		return begin;
	}
}//namaspace hbp