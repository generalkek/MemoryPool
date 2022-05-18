#include "utils.h"

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