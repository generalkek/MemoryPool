#ifndef MEM_POOL_UTILS
#define MEM_POOL_UTILS

#include <iostream>
#include <iomanip>
#include <ctime>

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
			double res = (temp - start) / static_cast<double>(CLOCKS_PER_SEC);
			start = temp;
			return res;
		}
	private:
		clock_t start;
	};

	template<unsigned int Size>
	void timingTest()
	{
		const unsigned int iteration = 100000;
		const unsigned int repetition = 10;

		std::cout << std::setprecision(8);
		std::cout << "TimingTest with object of size: " << Size << "\n";

		A<Size>* ptr;

		std::cout << "\nStandard allocators\n";
		Timer t;

		for (int j = 0; j < repetition; j++)
		{
			for (int i = 0; i < iteration; i++)
			{
				ptr = (A<Size>*)std::malloc(sizeof(A<Size>));
				new (ptr) A<Size>();
				std::free(ptr);
			}
			std::cout << "malloc-free: repetition:[" << j << "]: seconds " << t.getDelt() << "\n";
		}

		std::cout << "\nCustom allocators\n";
		t.getDelt();

		for (int j = 0; j < repetition; j++)
		{
			for (int i = 0; i < iteration; i++)
			{
				ptr = new A<Size>;
				delete ptr;
			}
			std::cout << "new-delete: repetition:[" << j << "]: seconds " << t.getDelt() << "\n";
		}

		std::cout << "\n\n";
	}
}

#endif//MEM_POOL_UTILS