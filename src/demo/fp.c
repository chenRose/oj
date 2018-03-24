#include<stdio.h>

typedef int (*pf)(const char *) ;
typedef int (*pfn)();
int a()
{
	printf("a");
	return 0;
}


int b()
{
	printf("b");
}

int  main()
{
		b("hello");
		pf a = (pf)12345;
		return 0;
}
