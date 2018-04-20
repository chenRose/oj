#include<stdio.h>
#include<stdlib.h>
int a[10000000]={1};
void main()
{
	unsigned int i = -1u;
	printf("%d\n",i);
	for(i=0;i<10000000;i++)
	{
		a[i]=i;
	}	
}
