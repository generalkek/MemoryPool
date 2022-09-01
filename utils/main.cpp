#include "utils.h"

#if defined(PROJ_ALIGNED_POOL)

int main()
{
	align_pool::setupPoolManager();

	pool_utils::timingTest<4>();
	pool_utils::timingTest<16>();
	pool_utils::timingTest<64>();
	pool_utils::timingTest<128>();
	pool_utils::timingTest<256>();

	pool_utils::timingTest2<8>();
	pool_utils::timingTest2<16>();
	pool_utils::timingTest2<64>();
	pool_utils::timingTest2<128>();
	pool_utils::timingTest2<512>();

	return 0;
}

#elif defined(PROJ_STACK_BASED_POOL)

int main(int argc, char** arvg)
{
	//pool_utils::timingTest<4>();
	//pool_utils::timingTest<400>();
	//pool_utils::timingTest<4000>();

	pool_utils::timingTest2<12>();
	pool_utils::timingTest2<16>();
	pool_utils::timingTest2<64>();
	pool_utils::timingTest2<128>();
	pool_utils::timingTest2<512>();

	return 0;
}

#elif defined(PROJ_HEAP_BASED_POOL)

int main()
{
	pool_utils::timingTestHandle<4>();
	pool_utils::timingTestHandle<16>();
	pool_utils::timingTestHandle<64>();
	pool_utils::timingTestHandle<256>();

	pool_utils::HandleTestReinitFeature();
	pool_utils::HandleTestDefragmentationFeature();
	return 0;
}

#endif