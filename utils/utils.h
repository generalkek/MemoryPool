#ifndef MEM_POOL_UTILS
#define MEM_POOL_UTILS

#include <ctime>
#include <iostream>
#include <iomanip>
#include <vector>

#if defined(PROJ_ALIGNED_POOL)

#include "../AlignedPool/src/ap.h"
typedef align_pool::AlignedPool CustomPool;
const char* g_poolName = "Aligned Pool";

#elif defined(PROJ_STACK_BASED_POOL)

#include "../StackBasedPool/src/sbp.h"
typedef sbp::StackBasedPool CustomPool;
const char* g_poolName = "Stack Based";

#elif defined(PROJ_HEAP_BASED_POOL)

#include "../HeapBasedPool/src/hbp.h"
#include "../HeapBasedPool/src/Handle.h"
typedef hbp::HeapStorage CustomPool;
const char* g_poolName = "Heap Storage";

#endif//PROJ_HEAP_BASED_POOL

namespace pool_utils
{
	template<unsigned int Size = 500>
	struct A
	{
		A()
		{
			int dummy = 1;
			dummy++;
		}
		char data[Size];
	};

	struct Timer
	{
		Timer()
		{
			start = std::clock();
		}

		double getDelt()
		{
			clock_t temp = std::clock();
			float res = (temp - start) / static_cast<float>(CLOCKS_PER_SEC);
			start = temp;
			return res;
		}
	private:
		clock_t start;
	};

#if defined(PROJ_ALIGNED_POOL)
	template<unsigned int Size>
	void timingTest()
	{
		const unsigned int iteration = 18000;
		const unsigned int repetition = 10;

		std::cout << std::setprecision(8);
		std::cout << "TimingTest with object of size: " << Size << "\n";

		std::vector<A<Size> > vecStdAlloc;
		std::vector<A<Size>, align_pool::AlignedPoolAllocator<A<Size> > > vecMyAlloc;

		std::cout << "\nVector with standard allocator\n";
		Timer t;

		double temp = 0.0;
		double min = 1.0;
		double max = 0.0;
		double accum = 0.0;
		
		t.getDelt();

		//---------------------------------------------------------------------
		//std::allocator
		//---------------------------------------------------------------------
		for (int j = 0; j < repetition; j++)
		{
			for (int i = 0; i < iteration; i++)
			{
				vecStdAlloc.push_back(A<Size>());
			}
			vecStdAlloc.clear();
			
			temp = t.getDelt();
			min = min < temp ? min : temp;
			max = max > temp ? max : temp;
			accum += temp;
			std::cout << "push_back in vector with std::allocator: repetition:[" << j << "]: seconds " << temp << "\n";
		}
		std::cout << "Min time:[" << min << "]\n";
		std::cout << "Max time:[" << max << "]\n";
		std::cout << "Average time:[" << accum / repetition << "]\n";

		min = 1.0;
		max = 0.0;
		accum = 0.0;

		std::cout << "\nCustom allocators\n";
		t.getDelt();

		//---------------------------------------------------------------------
		//align_pool::AlignedPoolAllocator
		//---------------------------------------------------------------------
		for (int j = 0; j < repetition; j++)
		{
			for (int i = 0; i < iteration; i++)
			{
				vecMyAlloc.push_back(A<Size>());
			}
			vecMyAlloc.clear();
			temp = t.getDelt();
			min = min < temp ? min : temp;
			max = max > temp ? max : temp;
			accum += temp;
			std::cout << "push_back in vector with align_poll:allocator: repetition:[" << j << "]: seconds " << temp << "\n";
		}
		std::cout << "Min time:[" << min << "]\n";
		std::cout << "Max time:[" << max << "]\n";
		std::cout << "Average time:[" << accum / repetition << "]\n";
		std::cout << "\n\n";
	}
#endif

	template<unsigned int Size>
	void randomDeleteTest()
	{
		const unsigned int arr_size = 5;

		std::cout << std::setprecision(8);
		std::cout << "TimingTest with object of size: " << Size << "\n";

		A<Size>* ptr[arr_size];
		int arr[arr_size]{ 0 };

		ptr[0] = new A<Size>;
		ptr[1] = new A<Size>;
		ptr[2] = new A<Size>;
		delete ptr[1];
		ptr[3] = new A<Size>;
		ptr[4] = new A<Size>;

	}

	template<unsigned int Size>
	void timingTest2()
	{
		typedef pool_utils::A<Size> MyType;
		const size_t repetion = 1000;
		const size_t arraySize = 2000;
		MyType* arr[arraySize]{nullptr};

#if defined(PROJ_ALIGNED_POOL)
		CustomPool loc{ Size, arraySize };
#elif defined(PROJ_STACK_BASED_POOL)
		CustomPool loc{};
#elif defined(PROJ_HEAP_BASED_POOL)
		CustomPool& loc = hbp::GetHeapStorage();
		loc.init(2000 * 150);
		loc.DEBUG_DumpAllFreeMemory();

#endif
		std::cout << "Timing test for array of objects with size:[" <<
			sizeof(MyType*) << "] and align[" << alignof(MyType*) << "]\n";
		
		Timer t;

		t.getDelt();
		//---------------------------------------------------------------------
		// malloc-free
		//---------------------------------------------------------------------
		for (int j = 0; j < repetion; j++)
		{
			for (int i = 0; i < arraySize; i++)
			{
				*(arr + i) = (MyType*)std::malloc(Size);
				(*(arr + i))->data[0] = 'a';
			}

			for (int i = 0; i < arraySize; i++)
			{
				std::free(*(arr + i));
			}
		}
		std::cout << "malloc-free time: " << t.getDelt() << "\n";
		
		t.getDelt();
		//---------------------------------------------------------------------
		// malloc_n-free_n
		//---------------------------------------------------------------------
		for (int j = 0; j < repetion; j++)
		{
			void* first = std::malloc(Size * arraySize);
			for (int i = 0; i < arraySize; i++)
			{
				*(arr + i) = static_cast<MyType*>(first) + i;
			}
			for (int k = 0; k < arraySize; k++)
				arr[k]->data[0] = 'a';
			std::free(first);
		}
		std::cout << "malloc_n-free_n time: " << t.getDelt() << "\n";
		
		t.getDelt();
		//---------------------------------------------------------------------
		// AlignedPool malloc-free
		//---------------------------------------------------------------------
		for (int j = 0; j < repetion; j++)
		{
			for (int i = 0; i < arraySize; i++)
			{
#if defined(PROJ_ALIGNED_POOL)
				*(arr + i) = (MyType*)loc.malloc();
#elif defined(PROJ_STACK_BASED_POOL) | defined (PROJ_HEAP_BASED_POOL)
				* (arr + i) = (MyType*)loc.malloc(sizeof(MyType));
#endif
				(*(arr + i))->data[0] = 'a';
			}
			
			for (int k = 0; k < arraySize; k++)
			{
				loc.free(*(arr + k));
			}
		}
		std::cout << g_poolName << " malloc-free time: " << t.getDelt() << "\n";
		
		t.getDelt();
		loc.DEBUG_DumpAllFreeMemory();

		//---------------------------------------------------------------------
		// AlignedPool malloc_n-free
		//---------------------------------------------------------------------
		for (int j = 0; j < repetion; j++)
		{
#if defined(PROJ_ALIGNED_POOL)
			* arr = (MyType*)loc.malloc_n(arraySize);
#elif defined(PROJ_STACK_BASED_POOL) | defined (PROJ_HEAP_BASED_POOL)
			MyType* first = (MyType*)loc.malloc(sizeof(MyType) * arraySize);
			for (int i = 0; i < arraySize; i++)
			{
				*(arr + i) = first + i;
			}
#endif
			for (int k = 0; k < arraySize; k++)
				arr[k]->data[0] = 'a';
#if defined (PROJ_HEAP_BASED_POOL)
			loc.free(*arr);
#else
			for (int i = 0; i < arraySize; i++)
			{
				loc.free(*(arr + i));
			}
#endif
		}
		std::cout << g_poolName << " malloc_n-free_n time: " << t.getDelt() << "\n";

		t.getDelt();
		loc.DEBUG_DumpAllFreeMemory();


#if defined(PROJ_ALIGNED_POOL)
		//---------------------------------------------------------------------
		// AlignedPoolManager malloc-free
		//---------------------------------------------------------------------
		for (int j = 0; j < repetion; j++)
		{
			for (int i = 0; i < arraySize; i++)
			{
				*(arr + i) = (MyType*)align_pool::GetAlignedPoolManager().malloc(Size);
				(*(arr + i))->data[0] = 'a';
			}
			
			for (int i = 0; i < arraySize; i++)
			{
				align_pool::GetAlignedPoolManager().free(*(arr + i));
			}
		}
		std::cout << "AlignedPoolManager time: " << t.getDelt() << "\n";

		//---------------------------------------------------------------------
		// AlignedPoolManager malloc_n-free_n
		//---------------------------------------------------------------------
		for (int j = 0; j < repetion; j++)
		{
			*(arr) = (MyType*)align_pool::GetAlignedPoolManager().malloc_n(Size, arraySize);
			
			for (int k = 0; k < arraySize; k++)
				arr[k]->data[0] = 'a';

			align_pool::GetAlignedPoolManager().free_n(*(arr), arraySize);
		}
		std::cout << "AlignedPoolManager time: " << t.getDelt() << "\n";
		std::cout << "------------------------------------------\n\n";
#endif //PROJ_ALIGNED_POOL
	}

#if defined(PROJ_HEAP_BASED_POOL)
	template <typename C, typename _Result = hbp::helpers::GetHandleType_t<std::remove_reference_t<C>>>
	_Result * GetObjPtr(hbp::HeapStorage & storage, const C*)
	{
		return static_cast<_Result*>(storage.malloc(sizeof(_Result)));
	}

	template <typename C>
	decltype(auto) GetObjPtr(hbp::HeapStorage & storage)
	{
		return static_cast<C*>(storage.malloc(sizeof(C)));
	}

	template <typename T>
	hbp::Handle<T>* makeHandleT(hbp::HeapStorage& store, size_t handleNumber = 1)
	{
		return static_cast<hbp::Handle<T>*>(hbp::helpers::makeHandle(store.malloc(handleNumber * sizeof(T)), handleNumber, sizeof(T)));
	}

	template <typename T>
	using HandleVec = std::vector< hbp::Handle<T>, hbp::HandleAllocator<hbp::Handle<T> > >;


	template<unsigned int Size>
	void timingTestHandle()
	{
		using namespace hbp::helpers;
		typedef pool_utils::A<Size> ValType;
		typedef hbp::Handle<ValType> HandleType;
		
		const int arrSize = 2000;
		const int repetion = 100;

		std::cout << "*************************************************************\n";
		std::cout << "timingTestHandle with Size[" << Size <<  "]\n";
		std::cout << "*************************************************************\n";

		CustomPool& heap = hbp::GetHeapStorage();
		heap.init(arrSize * Size * 4);
		HandleType* hArr[arrSize]{ nullptr };

		std::cout << "*************************************************************\n";
		std::cout << "Init Heap\n";
		std::cout << "*************************************************************\n";
		heap.DEBUG_DumpAllFreeMemory();

		Timer t;

		std::cout << "*************************************************************\n";
		std::cout << "makeHandle -> destroyHandle\n";
		std::cout << "*************************************************************\n";

		t.getDelt();

		for (int j = 0; j < repetion; j++)
		{
			HandleType* first = makeHandleT<ValType>(heap, arrSize);

			for (int i = 0; i < arrSize; i++)
			{
				hArr[i] = first + i;
				hRef(hArr[i]).data[0] = i % 255;
			}

			heap.free(destroyHandle(first, arrSize));
		}

		std::cout << "Handle constuction->operation->destruction " << t.getDelt() << "\n";

		std::cout << "*************************************************************\n";
		std::cout << "Begin HandleVec\n";
		std::cout << "*************************************************************\n";

		HandleVec<ValType> vec{};

		t.getDelt();

		for (int j = 0; j < repetion; j++)
		{
			vec.reserve(arrSize);

			for (int i = 0; i < arrSize; i++)
			{
				vec.push_back(std::move(HandleType{static_cast<ValType*>( heap.malloc(sizeof(ValType))) } ));
			}
			
			for (auto& h : vec)
			{
				hRef(h).data[0] = j % 255;
			}

			vec.clear();
		}

		std::cout << "HandleVec constuction->operation->destruction " << t.getDelt() << "\n";

		heap.DEBUG_DumpAllFreeMemory();
		std::cout << "*************************************************************\n";
		std::cout << "Destroy Heap\n";
		std::cout << "*************************************************************\n";
		heap.cleanAll();
	}

	void HandleTestReinitFeature()
	{
		using namespace hbp::helpers;
		typedef pool_utils::A<64> ValType;
		typedef hbp::Handle<int> HandleType;

		hbp::HeapStorage& heap = hbp::GetHeapStorage();
		heap.init(100);

		HandleVec<int> vecI;
		for (int i = 0; i < 10; i++)
		{
			vecI.push_back(hbp::Handle<int>{ GetObjPtr(heap, &vecI) } );
			hRef(vecI.back()) = i;
		}

		for (auto& x : vecI)
		{
			std::cout << hRef(x) << "\n";
		}

		// here happens reinit
		hbp::Handle<ValType> bigHandle{ GetObjPtr<ValType>(heap) };

		for (auto& x : vecI)
		{
			std::cout << hRef(x) << "\n";
		}

		heap.cleanAll();
	}

	void HandleTestDefragmentationFeature()
	{
		using namespace hbp::helpers;
		typedef pool_utils::A<64> ValType;
		typedef pool_utils::A<128> ValTypeX2;
		typedef hbp::Handle<int> HandleType;

		hbp::HeapStorage& heap = hbp::GetHeapStorage();
		heap.init(250);

		heap.DEBUG_DumpAllFreeMemory();

		hbp::Handle<ValType> bigDummy{ GetObjPtr<ValType>(heap) };

		heap.DEBUG_DumpAllFreeMemory();

		HandleVec<int> vecI;
		for (int i = 0; i < 10; i++)
		{
			vecI.push_back(hbp::Handle<int>{ GetObjPtr(heap, &vecI) });
			hRef(vecI.back()) = i;
		}

		heap.DEBUG_DumpAllFreeMemory();

		HandleType dummy{ GetObjPtr<int>(heap) };

		heap.DEBUG_DumpAllFreeMemory();

		heap.free(hPtr(bigDummy));
		bigDummy.~Handle();

		heap.DEBUG_DumpAllFreeMemory();

		hbp::Handle<ValTypeX2> bigDummyX2{ GetObjPtr<ValTypeX2>(heap) };

		for (auto& x : vecI)
		{
			std::cout << hRef(x) << "\n";
		}

		heap.DEBUG_DumpAllFreeMemory();

		heap.cleanAll();
	}

#endif //PROJ_HEAP_BASED_POOL

}//pool_utils

#endif//MEM_POOL_UTILS