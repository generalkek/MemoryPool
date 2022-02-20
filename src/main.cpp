#include "sbp.h"

#include <iostream>
#include <ctime>

struct A
{
	char data[50];
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
	const unsigned int iteration = 100000;

	Timer t;
	A* ptr;
	for (int i = 0; i < iteration; i++)
	{
		ptr = (A*)std::malloc(sizeof(A));
		std::free(ptr);
	}

	std::cout << t.getDelt() << "\n";

	for (int i = 0; i < iteration; i++)
	{
		ptr = new A;
		delete ptr;
	}

	std::cout << t.getDelt() << "\n";

}

int main(int argc, char** arvg)
{
	timingTest();

	return 0;
}