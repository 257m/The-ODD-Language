#include <stdio.h>
#include <stdlib.h>

double printd(double d)
{
	printf("%f\n",d);
	return d;
}

double getd() 
{
	char input [384] = "";
	fgets(input,384,stdin);
	return atof(input);
}

int printi(int i) {
	printf("%d\n",i);
	return i;
}

int geti() {
	char input [384] = "";
	fgets(input,384,stdin);
	return atoi(input);
}