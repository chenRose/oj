#include<stdio.h>
#include<stdlib.h>

int main()
{
	int num =5;
	int i;
	pid_t pid;
	for(i=0;i<num;i++)
	{
		pid = fork();
		if(pid==0)	//child 
		{
			int tmep = i;
			printf("[pid : %d]:%d\n",getpid(),i);
			sleep(1);
			printf("[pid : %d]:%d\n",getpid(),i);
			exit(0);
		}

	}
	sleep(20);
	return 0;
}
