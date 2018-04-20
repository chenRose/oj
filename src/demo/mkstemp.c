#include<stdlib.h>
#include<stdio.h>

void main()
{
	int fd;
	char temp[1024]="/1/123/21/23/1XXXXXX.c";
	fd = mkstemp(temp);
	if(fd<0)
	{
		printf("error");
	}
	printf("good file :%s ,fd is :%d\n",temp,fd);
	
	
}
