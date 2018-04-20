#include<stdio.h>
#include<stdlib.h>
#include<string.h>
void main()
{
	pid_t pid;
	pid = fork();
	if(pid==0)
	{
		fprintf(stderr,"before");
		freopen("/dev/tty","a+",stderr);
		const char * ptr = NULL;
		fprintf(stderr,"World!");
		printf("%s\n",ptr);
		exit(0);
	}
	else
	{
		int status;
		printf("parent...\n");
		wait(&status);
		if(WIFEXITED(status))
		{
			printf("正常终止。退出状态是：%d\n",WEXITSTATUS(status));
		}
		else if(WIFSIGNALED(status))
		{
			printf("信号引起停止...\n");
			int signal = WTERMSIG(status);
			printf("异常终止子进程。信号编号为：%d[%d][%s]\n",signal, WEXITSTATUS(status),strsignal(signal));	
		}
	}
}

