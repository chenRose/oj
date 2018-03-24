#include<stdio.h>
#include<stdlib.h>
void main()
{
	char str[1024]={0};
	strcpy(str,"helloworld");
	printf(str);
	printf("\n");
	strcpy(str,"XXXXX");
	printf(str);
	printf("\n");

	printf(str+6);
	
}
