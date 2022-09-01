#ifndef HEAP_BASED_POOL_SRC_HANDLE
#define HEAP_BASED_POOL_SRC_HANDLE
#include <map>

namespace hbp {

	typedef ptrdiff_t				Int;
	typedef const ptrdiff_t			CInt;
	typedef size_t					UInt;
	typedef const size_t			CUInt;
	typedef std::map<UInt, void*>	HandleTable;

	constexpr static UInt s_invalidId = ~0;
	constexpr static UInt s_maxNumberOfHandles = 1000000u;

	class IHandle
	{
	public:
				IHandle();
				IHandle(void* ptr);
				IHandle(const IHandle&) = delete;
				IHandle(IHandle&& other) noexcept;
				virtual ~IHandle();

				void operator=(IHandle&& other) noexcept;
				void operator=(void* ptr);
	protected:

				void*	get() const;

	private:
				void	release();
		UInt	m_id;
	};

	template<typename T>
	class Handle : public IHandle
	{
	public:
		typedef T MyType;

	public:
				Handle():IHandle() 
					{};
				Handle(T* ptr) :IHandle{ static_cast<void*>(ptr) }
					{};
				Handle(Handle<T>&& other) noexcept
				:IHandle(std::move(*static_cast<IHandle*>(&other)))	
					{};
				~Handle() 
					{};

				void operator=(Handle<T>&& other) noexcept
				{
					IHandle::operator=(std::move(*static_cast<IHandle*>(&other)));
				}

				template<typename MyType>
				MyType* pointer() const
				{
					return static_cast<MyType*>(get());
				}

				template<typename MyType>
				MyType value() const
				{
					return *(pointer<MyType>());
				}

				template<typename MyType>
				MyType& ref() const
				{
					return *(pointer<MyType>());
				}
	};
	
	class HandleManager final
	{
	public:
		typedef HandleTable::iterator iterator;
		
	public:
						HandleManager();
						~HandleManager();

		void*			getHandle(CUInt id) const;
		void			insert(CUInt id, void* ptr);
		void			erase(CUInt id);

		void			replace(CUInt id, void* ptr);

		iterator		begin() { return m_handles.begin(); };
		iterator		end() { return m_handles.end(); }

		static UInt		GenerateNewId();
	private:
		HandleTable		m_handles;
	};

	HandleManager& GetHandleManager();
	
	namespace helpers {

		IHandle* makeHandle(void* obj, CUInt handlesNumber = 1, CUInt objOffset = 0);

		void* destroyHandle(IHandle* ptr, CUInt handlesNumber = 1);

		template<typename MyType>
		MyType hVal(Handle<MyType>* handle)
		{
			return handle->value<MyType>();
		}

		template<typename MyType>
		MyType* hPtr(Handle<MyType>* handle)
		{
			return handle->pointer<MyType>();
		}

		template<typename MyType>
		MyType& hRef(Handle<MyType>* handle)
		{
			return handle->ref<MyType>();
		}

		template<typename MyType>
		MyType hVal(Handle<MyType> & handle)
		{
			return handle.value<MyType>();
		}

		template<typename MyType>
		MyType* hPtr(Handle<MyType> & handle)
		{
			return handle.pointer<MyType>();
		}

		template<typename MyType>
		MyType& hRef(Handle<MyType> & handle)
		{
			return handle.ref<MyType>();
		}

		template<typename T>
		struct GetHandleType {
			typedef T type;
		};

		template<typename T>
		struct GetHandleType<Handle<T>>
		{
			typedef T type;
		};

		template<typename C>
		using GetHandleType_t = typename GetHandleType<typename C::value_type>::type;

	}//namespace helpers

	//defined in hbp.cpp
	extern void handleRelease(void* ptr);

	//generic HandleAllocator for different STL stuff such as 
	//_Container_base12, _Iterator_base12, _Container_proxy etc. 
	template<typename T>
	class HandleAllocator
	{
	public:
		typedef T			value_type;

		typedef T*			pointer;
		typedef const T*	const_pointer;

		typedef T&			reference;
		typedef const T&	const_reference;

		typedef size_t				size_type;
		typedef int					difference_type;

		template <typename U>
		struct rebind
		{
			typedef HandleAllocator<U> other;
		};

	public:
		HandleAllocator() {};

		template <typename U>
		HandleAllocator(const HandleAllocator<U>& other) {};

		HandleAllocator& operator=(const HandleAllocator& other) {};


		static pointer address(reference r) { return &r; }
		static const_pointer address(const_reference r) { return &r; }

		static pointer allocate(const size_type n, const void* = 0)
		{
			return static_cast<pointer>(::operator new(std::_Get_size_of_n<sizeof(value_type)>(n)));
			//return helpers::makeHandle(nullptr, n);
		}

		static void deallocate(pointer ptr, const size_type blockNumber)
		{
			::operator delete(ptr, std::_Get_size_of_n<sizeof(value_type)>(blockNumber));
			/*for (size_type i = 0; i < blockNumber; i++)
			{
				helpers::destroyHandle(ptr + i);
			}*/
		}

		static  void construct(pointer const p, value_type&& t)
		{
			new (static_cast<void*>(p)) value_type(std::forward<value_type>(t));
		}

		static  void destroy(const_pointer ptr)
		{
			ptr->~value_type();
		}

		bool operator==(const HandleAllocator&) const { return true; }
		bool operator!=(const HandleAllocator&) const { return false; }

		constexpr static size_type max_size() noexcept
		{
			return static_cast<size_type>(-1) / sizeof(value_type);
		}
	};

	//specialization for any Handle
	template<typename T>
	class HandleAllocator<Handle<T>>
	{
	public:
		typedef Handle<T>			value_type;

		typedef Handle<T>*			pointer;
		typedef const Handle<T>*	const_pointer;

		typedef Handle<T>&			reference;
		typedef const Handle<T>&	const_reference;

		typedef size_t				size_type;
		typedef int					difference_type;

		template <typename U>
		struct rebind
		{
			typedef HandleAllocator<U> other;
		};

	public:
		HandleAllocator() {};

		template <typename U>
		HandleAllocator(const HandleAllocator<U>& other) {};

		HandleAllocator& operator=(const HandleAllocator& other) {};


		static pointer address(reference r) { return &r; }
		static const_pointer address(const_reference r) { return &r; }

		static pointer allocate(const size_type n, const void* = 0)
		{
			return static_cast<pointer>(helpers::makeHandle(nullptr, n));
		}

		static void deallocate(pointer ptr, const size_type blockNumber)
		{
			handleRelease(helpers::destroyHandle(ptr, blockNumber));
		}

		static  void construct(pointer const p, value_type&& t)
		{
			new (static_cast<void*>(p)) value_type(std::forward<value_type>(t));
		}

		static  void destroy(const_pointer ptr)
		{
			handleRelease(ptr->pointer<T>());
			ptr->~value_type();
		}

		bool operator==(const HandleAllocator&) const { return true; }
		bool operator!=(const HandleAllocator&) const { return false; }

		constexpr static size_type max_size() noexcept
		{
			return static_cast<size_type>(-1) / sizeof(value_type);
		}
	};
}//namespace hbp

#endif//HEAP_BASED_POOL_SRC_HANDLE
