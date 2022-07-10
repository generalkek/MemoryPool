#include "hbp.h"
#include <memory>

int main()
{
	int s = 1000;
	hbp::HeapStorageHandles storage;
	storage.init(s);

	const int arrS = 10;
	hbp::Handle* pint[arrS];

	for (int i = 0; i < arrS; i++)
	{
		pint[i] = storage.malloc(sizeof(int));
		pint[i]->operator-><int>() = i;
	}
	
	pint[7]->operator-><int>() = 12324;

	for (int i = 0; i < arrS; i++)
	{
		printf_s("Value:[%d]\n", pint[i]->value<int>());
	}

	for (int i = arrS - 1; i >= 0; i--)
	{
		storage.free(pint[i]);
	}

	return 0;
}