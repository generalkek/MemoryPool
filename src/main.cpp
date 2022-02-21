#include "sbp.h"

#include <iostream>
#include <ctime>
#include <iomanip>

struct A
{
	A()
	{
		int dummy = 1;
		dummy++;
	}
	char data[500000];
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

void timingTest()
{
	const unsigned int iteration = 100;
	const unsigned int repetition = 10;

	std::cout << std::setprecision(8);

	Timer t;
	A* ptr;
	for (int j = 0; j < repetition; j++)
	{
		for (int i = 0; i < iteration; i++)
		{
			ptr = (A*)std::malloc(sizeof(A));
			new (ptr) A();
			std::free(ptr);
		}
		std::cout <<  "malloc-free: repetition:[" << j << "]: seconds " << t.getDelt() << "\n";
	}

	for (int j = 0; j < repetition; j++)
	{
		for (int i = 0; i < iteration; i++)
		{
			ptr = new A;
			delete ptr;
		}
		std::cout <<  "new-delete: repetition:[" << j << "]: seconds " << t.getDelt() << "\n";
	}

}

void incorectFreeOrderTest()
{
	int* _1 = new int(4);
	int* _2 = new int(4);
	int* _3 = new int(4);
	int* _4 = new int(4);

	//first three will not be freed, because of wrong order
	//nonetheless memory will be freed in pool.~StackBasedPool()
	delete (_1);
	delete (_2);
	delete (_3);

	//last will be propperly freed
	delete (_4);
}

int main(int argc, char** arvg)
{
	timingTest();
	incorectFreeOrderTest();

	return 0;
}