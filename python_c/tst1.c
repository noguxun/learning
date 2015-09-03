#include "stdio.h"

#define TST_DATA_LEN 10
unsigned char test_data[TST_DATA_LEN] = {0,1,2,3,4,5,6,7,8,8};

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

unsigned char * GetBytes(int *pLen)
{
	if(pLen) {
		*pLen = TST_DATA_LEN;
	}
	return test_data;
}

void GetArrayData(int size, int data[])
{
	int i;
	for(i = 0; i < size; i++) {
		data[i] = i + 1;
	}
}
