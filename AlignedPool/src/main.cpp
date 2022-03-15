#include "ap.h"

#include "../../utils/utils.h"

int main()
{
	pool_utils::timingTest<12>();
	pool_utils::timingTest<16>();
	pool_utils::timingTest<64>();
	pool_utils::timingTest<4000>();
	//pool_utils::randomDeleteTest<8>();
	//pool_utils::randomDeleteTest<128>();
	//pool_utils::randomDeleteTest<24>();
	return 0;
}