#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "vgc_message.h"
#include "vgc_mprotect.h"
#include "vgc_malloc.h"

void pippo(void);


void aa(void)
{
	pippo();
}


void pippo(void)
{
	char *z = vgc_malloc(20);
	printf("z:0x%lx\n", (unsigned long int)z);
//	vgc_free(z);
}


int main(void)
{
	printf("Start\n");
#if 1
	char *a = vgc_malloc(10);
	printf("a:0x%lx\n", (unsigned long int)a);
//	vgc_free(a);
//	pippo();
	aa();
#endif
#if 1
	char *b = vgc_malloc(10);
	printf("b:0x%lx\n", (unsigned long int)b);
	vgc_free(b);
#endif
#if 0
	// This part shows the error with mprotect that probably is resolved by pkey_mprotect()
	for (int i = 0; i < 100000; i++) {
		char *d = vgc_malloc(10);
		if (d == 0) exit(1);
	}
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
		printf("Started child\n");	// Ma con pkey_mprotect() serve la chiamata sotto? Provare con un processo child se vede la memoria protetta
		startChildMprotect();	// Questa deve essere chiamata dalle funzioni di init con un signal per ogni nuovo fork
		char *c = vgc_malloc(20);
		printf("c:0x%lx\n", (unsigned long int)c);
//		vgc_free(c);
		return 0;
	}
#endif
	printf("End\n");
}
