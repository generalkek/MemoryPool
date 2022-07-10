#include "Handle.h"

namespace hbp
{
	HandleManager g_handleManager{};
	Uint g_id{ 0u };

	//-----------------------------------------------------------
	void ResetHandlesId()
	{
		g_id = 0u;
	}

	//-----------------------------------------------------------
	HandleManager& GetHandleManager()
	{
		return g_handleManager;
	}

	//-----------------------------------------------------------
	Handle::Handle()
		:m_id{ s_invalidId }
	{
	}

	//-----------------------------------------------------------
	Handle::Handle(void* ptr)
	{
		m_id = HandleManager::GenerateNewId();
		GetHandleManager().insert(m_id, ptr);
	}

	//-----------------------------------------------------------
	Handle::~Handle()
	{
		GetHandleManager().erase(m_id);
	}

	//-----------------------------------------------------------
	void* Handle::get() const
	{
		return GetHandleManager().getHandle(m_id);
	}

	//-----------------------------------------------------------
	HandleManager::HandleManager()
	{
	}

	//-----------------------------------------------------------
	HandleManager::~HandleManager()
	{
		m_handles.clear();
	}

	//-----------------------------------------------------------
	void* HandleManager::getHandle(CUint id) const
	{
		void* res = nullptr;
		HandleTable::const_iterator it = m_handles.find(id);
		if (it != m_handles.cend()) 
			res = it->second;
		return res;
	}

	//-----------------------------------------------------------
	void HandleManager::insert(CUint id, void* ptr)
	{
		if (m_handles.find(id) != m_handles.cend()){
			printf_s("There is already a handle with id[%d]\n", id);
		} else {
			m_handles[id] = ptr;
		}
	}

	//-----------------------------------------------------------
	void HandleManager::erase(CUint id)
	{
		if (m_handles.size() != 0 && m_handles.find(id) != m_handles.cend()) {
			m_handles.erase(id);
			if (m_handles.size() == 0) {
				ResetHandlesId();
			}
		}
	}

	//-----------------------------------------------------------
	Uint HandleManager::GenerateNewId()
	{
		return g_id++;
	}
}// namespace hbp