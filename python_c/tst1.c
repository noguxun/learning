#include "stdio.h"

void PrintHello(void)
{
    printf("Hello, Matrix \n");	
}

void PrintMyWord(char *p)
{
    printf("%s \n", p);	
}


int GetInt1(int input)
{
	return input + 100;
}

int GetInt2(int input1, int input2)
{
	return input1 + input2;
}

char* GetString(void)
{
	return "Dont buy shit we dont need";
}
