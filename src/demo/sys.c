#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<errno.h>
void main()
{
	int fd = open("ciiy",O_RDONLY);
		
	char buf[1024];
	printf("\033[4mfd:%d--errno = %d\033[0m",fd,errno);
    printf("syscall failed. errno = %ld\n", -(fd));
}
	if(fd>0)
	{
		int size = read(fd,buf,1024);
	
		printf("open return %d\nread return %d\n",fd,size);	
	}
}

