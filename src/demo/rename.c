#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
void main()
{
	char old_file[] = "hello.c";
	int fd = open(old_file,O_RDWR);
	printf("FD:%d\n",fd);
	
	rename("hello.c","world.c");
	write(fd,"hello???",20);
	
}
