#ifndef MEM_POOL_UTILS
#define MEM_POOL_UTILS

#include <ctime>
#include <iostream>
#include <iomanip>
#include <vector>

#if defined(PROJ_ALIGNED_POOL)
#include "../AlignedPool/src/ap.h"
typedef align_pool::AlignedPool CustomPool;
#elif defined(PROJ_STACK_BASED_POOL)
#include "../StackBasedPool/src/sbp.h"
typedef sbp::StackBasedPool CustomPool;
#elif defined(PROJ_HEAP_BASED_POOL)
#endif

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
		MyType* arr[arraySize];

#if defined(PROJ_ALIGNED_POOL)
		CustomPool loc{ Size, arraySize };
#elif defined(PROJ_STACK_BASED_POOL)
		CustomPool loc{};
#endif
		std::cout << "Timing test for array of objects with size:" << Size << "\n";

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

		//---------------------------------------------------------------------
		// AlignedPool malloc-free
		//---------------------------------------------------------------------
		for (int j = 0; j < repetion; j++)
		{
			for (int i = 0; i < arraySize; i++)
			{
#if defined(PROJ_ALIGNED_POOL)
				*(arr + i) = (MyType*)loc.malloc();
#elif defined(PROJ_STACK_BASED_POOL)
				* (arr + i) = (MyType*)loc.malloc(sizeof(MyType));
#endif
				(*(arr + i))->data[0] = 'a';
			}

			for (int i = 0; i < arraySize; i++)
			{
				loc.free(*(arr + i));
			}
		}
		std::cout << "AlignedPool malloc-free time: " << t.getDelt() << "\n";

		//---------------------------------------------------------------------
		// AlignedPool malloc_n-free
		//---------------------------------------------------------------------
		for (int j = 0; j < repetion; j++)
		{
#if defined(PROJ_ALIGNED_POOL)
			* arr = (MyType*)loc.malloc_n(arraySize);
#elif defined(PROJ_STACK_BASED_POOL)
			* arr = (MyType*)loc.malloc(arraySize);
#endif
			for (int k = 0; k < arraySize; k++)
				arr[k]->data[0] = 'a';

			for (int i = 0; i < arraySize; i++)
			{
				loc.free(*(arr + i));
			}
		}
		std::cout << "AlignedPool malloc_n-free time: " << t.getDelt() << "\n";

		//---------------------------------------------------------------------
		// AlignedPoolManager malloc-free
		//---------------------------------------------------------------------
		for (int j = 0; j < repetion; j++)
		{
			for (int i = 0; i < arraySize; i++)
			{
				*(arr + i) = (MyType*)align_pool::g_poolManager.malloc(Size);
				(*(arr + i))->data[0] = 'a';
			}
			
			for (int i = 0; i < arraySize; i++)
			{
				align_pool::g_poolManager.free(*(arr + i));
			}
		}
		std::cout << "AlignedPoolManager time: " << t.getDelt() << "\n";

		//---------------------------------------------------------------------
		// AlignedPoolManager malloc_n-free_n
		//---------------------------------------------------------------------
		for (int j = 0; j < repetion; j++)
		{
			*(arr) = (MyType*)align_pool::g_poolManager.malloc_n(Size, arraySize);
			
			for (int k = 0; k < arraySize; k++)
				arr[k]->data[0] = 'a';

			align_pool::g_poolManager.free_n(*(arr), arraySize);
		}
		std::cout << "AlignedPoolManager time: " << t.getDelt() << "\n";
		std::cout << "------------------------------------------\n\n";
	}
}

#endif//MEM_POOL_UTILS