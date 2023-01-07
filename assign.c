#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int* getpointer(int n)
{
	return &n;
}

int main()
{
	int x = 2;
	printf("%x\n",*getpointer(x));
	return 0;
}