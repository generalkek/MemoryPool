#include "sbp.h"

#include <iostream>

struct A
{
	int* a;
	A()
	{
		a = new int;
	}
	~A() 
	{
		std::cout << "~A()\n";
		delete a;
	};
};

int main(int argc, char** arvg)
{
	/*sbp::StackBasedPool pool;

	void* ptr1 = pool.malloc(10);
	void* ptr2 = pool.malloc(20);
	void* ptr3 = pool.malloc(40);
	void* ptr4 = pool.malloc(80);

	pool.free();
	pool.free();
	pool.free();
	pool.free();*/

	int* pi = new int(32);
	return 0;
}