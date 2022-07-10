#ifndef HEAP_BASED_POOL_SRC_HANDLE
#define HEAP_BASED_POOL_SRC_HANDLE
#include <map>

namespace hbp {

	typedef unsigned int			Uint;
	typedef const unsigned int		CUint;
	typedef std::map<Uint, void*>	HandleTable;

	constexpr static Uint s_invalidId = ~0;
	
	class Handle
	{
	public:
						Handle();
						Handle(void* ptr);
						~Handle();

						template<typename T>
						T value() const
						{
							return *(this->operator*<T>());
						}

						template<typename T>
						T* operator*() const
						{
							return static_cast<T*>(get());
						}

						template<typename T>
						T& operator->() const
						{
							return *(this->operator*<T>());
						}
	private:

		void*			get() const;

	private:
		Uint			m_id;
	};

	class HandleManager
	{
	public:
						HandleManager();
						~HandleManager();

		void*			getHandle(CUint id) const;
		void			insert(CUint id, void* ptr);
		void			erase(CUint id);

		static Uint		GenerateNewId();
	private:
		HandleTable		m_handles;
	};

	HandleManager& GetHandleManager();

}//namespace hbp
#endif//HEAP_BASED_POOL_SRC_HANDLE
