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

using namespace hbp::helpers;

template <typename T>
using HandleVec = std::vector< hbp::Handle<T>, hbp::HandleAllocator<hbp::Handle<T> > >;


template <typename C, typename _Result = GetHandleType_t<std::remove_reference_t<C>>>
_Result* GetObjPtr(hbp::HeapStorage& storage, const C&)
{
	return static_cast<_Result*>(storage.malloc(sizeof(_Result)));
}

int main()
{
	int s = 1000;
	hbp::HeapStorage storage;
	storage.init(s);

	const int arrS = 10;
	HandleVec<int> handleContainer;

	handleContainer.reserve(10);

	for (int i = 0; i < arrS; i++)
	{
		handleContainer.push_back(GetObjPtr(storage, handleContainer));
		//handleContainer.push_back(static_cast<int*>(storage.malloc(sizeof(int))));
		hRef(&handleContainer.back()) = i;
	}
	
	for (auto& x : handleContainer)
	{
		printf_s("Value:[%d]\n", hRef(&x));
	}

	return 0;
}

#endif