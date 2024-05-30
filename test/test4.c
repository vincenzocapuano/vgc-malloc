// Test how many mprotect we can call, it is regulated by parameter /proc/sys/vm/max_map_count
//
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>


#define PAGESIZE 4096

int main(void)
{
	for (int i = 1; i < 750000; i++) {
		char *buffer = memalign(PAGESIZE, PAGESIZE * 4);
		int r = mprotect(buffer, PAGESIZE, PROT_NONE);
		if (r == -1) {
			printf("Stopped at: %i\n", i);
			return 1;
		}
	}

	return 0;
}
