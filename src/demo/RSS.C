#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
int get_proc_status(int pid, const char * mark) {
	FILE * pf;
	int BUFFER_SIZE = 1024;
	char fn[BUFFER_SIZE], buf[BUFFER_SIZE];
	int ret = 0;
	sprintf(fn, "/proc/%d/status", pid);
	pf = fopen(fn, "re");
	int m = strlen(mark);
	while (pf && fgets(buf, BUFFER_SIZE - 1, pf)) {

		buf[strlen(buf) - 1] = 0;
		if (strncmp(buf, mark, m) == 0) {
			sscanf(buf + m + 1, "%d", &ret);
		}
	}
	if (pf)
		fclose(pf);
	return ret;
}
int  main()
{
	int MB = 1024*1024;
	printf("程序刚开始\n");
	int rss,vmpeak;
	rss = get_proc_status(getpid(),"VmRSS:");
	vmpeak = get_proc_status(getpid(),"VmPeak:");
	printf("RSS:%d\nVmPeak:%d\n",rss,vmpeak);

	printf("%d申请100M：未使用\n",getpid());
	char *p =(char*) malloc(100*MB);
    rss = get_proc_status(getpid(),"VmRSS:");
    vmpeak = get_proc_status(getpid(),"VmPeak:");
    printf("RSS:%d\nVmPeak:%d\n",rss,vmpeak);

	printf("use 10M\n");

	int i ;
	for(i=0;i<10*MB;i++)
	{
		p[i]=i;
	}
	printf("已经使用10Mb\n");
	rss = get_proc_status(getpid(),"VmRSS:");
    vmpeak = get_proc_status(getpid(),"VmPeak:");
    printf("RSS:%d\nVmPeak:%d\n",rss,vmpeak);

	pause();
	

	//申请100M
	return 0;
	
}
