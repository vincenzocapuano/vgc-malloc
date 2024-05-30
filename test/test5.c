// Test pkey_mprotect()
//
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>


#define PAGE_SIZE 4096



int main(void)
{
	int real_prot = PROT_READ | PROT_WRITE;

	printf("\n");

	for (int i = 0; i < 1000000; i++) {
		int pkey = pkey_alloc(0, PKEY_DISABLE_WRITE);
		char *ptr = mmap(NULL, PAGE_SIZE, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
		int ret = pkey_mprotect(ptr, PAGE_SIZE, real_prot, pkey);

		if (ret) printf("Returning error");

		printf("-> %d\r", i);

		pkey_set(pkey, 0); // clear PKEY_DISABLE_WRITE
		*ptr = 10; // assign something
		pkey_set(pkey, PKEY_DISABLE_WRITE); // set PKEY_DISABLE_WRITE again


#if 0
		// Replace the mprotect() code below
		//
		mprotect(ptr, size, PROT_NONE);
#else
		// with
		//
		pkey = pkey_alloc(0, PKEY_DISABLE_WRITE | PKEY_DISABLE_ACCESS);
		pkey_mprotect(ptr, PAGE_SIZE, PROT_READ | PROT_WRITE, pkey);
#endif
		// Free the memory
		//
//		munmap(ptr, PAGE_SIZE);
//		pkey_free(pkey);
	}

	return 0;
}


