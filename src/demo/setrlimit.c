#include<stdio.h>
#include<stdlib.h>
#include<sys/resource.h>
#include<sys/signal.h>
#include<string.h>
void sig_ocpu(int sign)
{
	printf("收到信号\n");
	printf("%d[]\n",sign);
	char *text = strsignal(sign);
	int i = strsignal(sign);
	printf("%d\n",i);
	if(text!=NULL)
		printf("%s\n",strsignal(sign));
}

int main()
{
	int time_limit = 3;
	struct rlimit limit;	
	limit.rlim_max=time_limit*2;
	limit.rlim_cur=time_limit;
	setrlimit(RLIMIT_CPU,&limit);
	if(signal(SIGXCPU,sig_ocpu)==SIG_ERR)
	{
		printf("error\n");
	}
	printf("%d\n",getpid());
	while(1)
	{
	}
	
}
