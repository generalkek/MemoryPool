#include "sbp.h"

#include "../../utils/utils.h"


int main(int argc, char** arvg)
{
	pool_utils::timingTest<4>();
	pool_utils::timingTest<400>();
	pool_utils::timingTest<4000>();

	return 0;
}