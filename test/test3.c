#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "vgc_message.h"
#include "vgc_malloc.h"



int main(void)
{
	printf("Start\n");

	char *a = vgc_malloc(100);
	printf("a:0x%lx\n", (unsigned long int)a);

//	a[100] = 1;

//	vgc_free(a);

	printf("End\n");
}
