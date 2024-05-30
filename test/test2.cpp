#include <cstdlib>
#include <iostream>


#ifdef MEMMGR
#include <new>
#include "vgc_memoryManager.hpp"
MemoryManager vgcMemMgr;	// make singleton in library start
#else
#include "vgc_new.hpp"
#endif


int main()
{
	int *a = new int;
	delete a;

	class pippo {
		int a;
		char b;
	};

#ifdef MEMMGR
	pippo *b = new (vgcMemMgr) pippo;
#else
	pippo *b = new pippo;
#endif
}
