#include "Handle.h"

#include "ap.h"

namespace hbp
{
	using namespace helpers;

	static HandleManager g_handleManager{};
	static UInt g_id{ 0u };

	//-----------------------------------------------------------
	void customSetupAlignedPool()
	{
		align_pool::GetAlignedPoolManager().addPool(sizeof(IHandle), s_maxNumberOfHandles);
		align_pool::GetAlignedPoolManager().init();
	}

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
	IHandle::IHandle()
		:m_id{ s_invalidId }
	{
	}

	//-----------------------------------------------------------
	IHandle::IHandle(void* ptr)
		: m_id{ s_invalidId }
	{
		if (ptr) {
			m_id = HandleManager::GenerateNewId();
			GetHandleManager().insert(m_id, ptr);
		}
	}

	//-----------------------------------------------------------
	IHandle::IHandle(IHandle&& other) noexcept
	{
		release();
		this->m_id = other.m_id;
		other.m_id = s_invalidId;
	}

	//-----------------------------------------------------------
	IHandle::~IHandle()
	{
		release();
	}

	void IHandle::operator=(IHandle&& other) noexcept
	{
		release();
		this->m_id = other.m_id;
		other.m_id = s_invalidId;
	}

	void IHandle::operator=(void* ptr)
	{
		GetHandleManager().replace(m_id, ptr);
	}

	//-----------------------------------------------------------
	void* IHandle::get() const
	{
		return GetHandleManager().getHandle(m_id);
	}

	void IHandle::release()
	{
		if (m_id != s_invalidId)
		{
			GetHandleManager().erase(m_id);
			m_id = s_invalidId;
		}
	}

	//-----------------------------------------------------------
	IHandle* helpers::makeHandle(void* obj, CUInt handlesNumber, CUInt objOffset)
	{
		if (!align_pool::GetAlignedPoolManager().isInitialized()) {
			customSetupAlignedPool();
		}
		IHandle* h = static_cast<IHandle*>(align_pool::GetAlignedPoolManager().malloc_n(sizeof(IHandle), handlesNumber));
		char* objPtr = static_cast<char*>(obj);
		for (UInt i = 0; i < handlesNumber; i++, 
			objPtr ? objPtr += objOffset : objPtr) {
			new (h + i) IHandle{ objPtr};
		}
		return h;
	}

	//-----------------------------------------------------------
	void* helpers::destroyHandle(IHandle* ptr, CUInt handlesNumber)
	{
		if (!ptr)
			return nullptr;

		IHandle* first = ptr;
		void* obj = ptr ? hPtr(static_cast<Handle<void>*>(ptr)) : nullptr;
		for (UInt i = 0; i < handlesNumber; i++)
		{
			(first+i)->~IHandle();
		}
		align_pool::GetAlignedPoolManager().free_n(first, handlesNumber);
		return obj;
	}

	//-----------------------------------------------------------
	HandleManager::HandleManager()
	{}

	//-----------------------------------------------------------
	HandleManager::~HandleManager()
	{
		m_handles.clear();
	}

	//-----------------------------------------------------------
	void* HandleManager::getHandle(CUInt id) const
	{
		void* res = nullptr;
		HandleTable::const_iterator it = m_handles.find(id);
		if (it != m_handles.cend()) 
			res = it->second;
		return res;
	}

	//-----------------------------------------------------------
	void HandleManager::insert(CUInt id, void* ptr)
	{
		if (m_handles.find(id) != m_handles.cend()){
			printf_s("There is already a handle with id[%zd]\n", id);
		} else {
			m_handles[id] = ptr;
		}
	}

	//-----------------------------------------------------------
	void HandleManager::erase(CUInt id)
	{
		HandleTable::iterator it;
		if (m_handles.size() != 0 && 
			(it = m_handles.find(id)) != m_handles.cend()) {
			//delete item from table
			m_handles.erase(it);
			if (m_handles.size() == 0) {
				ResetHandlesId();
			}
		}
	}

	//-----------------------------------------------------------
	void HandleManager::replace(CUInt id, void* ptr)
	{
		if (m_handles.find(id) == m_handles.cend()) {
			printf_s("There isn't a handle with id[%zd]\n", id);
		}
		else {
			m_handles[id] = ptr;
		}
	}

	//-----------------------------------------------------------
	UInt HandleManager::GenerateNewId()
	{
		return g_id++;
	}
}// namespace hbp