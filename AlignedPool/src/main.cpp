#include "ap.h"

int main()
{
	align_pool::AlignedPool pool(4, 100);

	int* p1 = static_cast<int*>(pool.malloc(4));
	int* p2 = static_cast<int*>(pool.malloc(sizeof(int)));
	int* p3 = static_cast<int*>(pool.malloc(100));
	int* p4 = static_cast<int*>(pool.malloc(sizeof(int)));

	pool.free(p4);
	pool.free(p3);
	pool.free(p2);
	pool.free(p1);

	return 0;
}