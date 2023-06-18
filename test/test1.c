#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "vgc_message.h"
#include "vgc_malloc_mprotect.h"
#include "vgc_malloc.h"
#include "vgc_stacktrace.h"



void pippo(void)
{
	char *z = vgc_malloc(20);
	printf("z:0x%lx\n", (unsigned long int)z);
	vgc_free(z);
}


int main(void)
{
	printf("Start\n");
#if 1
	char *a = vgc_malloc(10);
	printf("a:0x%lx\n", (unsigned long int)a);
	vgc_free(a);
	pippo();
#endif
#if 0
	char *b = vgc_malloc(10);
	printf("b:0x%lx\n", (unsigned long int)b);
	vgc_free(b);
#endif
#if 0
	pid_t pID = fork();
	if (pID < 0) {
		// failed to fork
		exit(1);
	        // Throw exception
	}
	else if (pID > 0) {
		// parent
		// Code only executed by parent process
		//
//		sleep(1);
	}
	else {
		// child
		// Code only executed by child process
		//
		printf("Started child\n");
		startChildMprotect();	// Questa deve essere chiamata dalle funzioni di init con un signal per ogni nuovo fork
		char *c = vgc_malloc(20);
		printf("c:0x%lx\n", (unsigned long int)c);
//		vgc_free(c);
		return 0;
	}
#endif
	printf("End\n");
}
