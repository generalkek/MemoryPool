#include "utils.h"



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