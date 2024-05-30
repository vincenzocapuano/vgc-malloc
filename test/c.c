#include <stdio.h>

int main(void)
{
	int a = 4 * 1024;
	int A = (4 * 1024) - 1;
	int b = a * 3 - 5 + 0x10000;
	printf("b: 0x%x - c: 0x%x - d: 0x%x %d\n", b,      b & ~A,       b & A,       a - b & A);
}
