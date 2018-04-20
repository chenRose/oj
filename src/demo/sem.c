#include<stdio.h>
#include<stdlib.h>
#include<sys/sem.h>
#define SEMPERM 0600

typedef union _semun {
  int val;
  struct semid_ds *buf;
  ushort *array;
} semun;


int v(int semid)
{
  struct sembuf v_buf;

  v_buf.sem_num=0;
  v_buf.sem_op=1;    //信号量加1
  v_buf.sem_flg=SEM_UNDO;
  
  if (semop(semid, &v_buf, 1)==-1)
  {
    perror("v(semid)failed");
    exit(1);
  }
  return(0);
}
int p(int semid)
{
  struct sembuf p_buf;

  p_buf.sem_num=0;
  p_buf.sem_op=-1;        //信号量减1，注意这一行的1前面有个负号
  p_buf.sem_flg=SEM_UNDO;
  
  if (semop(semid, &p_buf, 1)==-1)   
  {
       perror("p(semid)failed");
       exit(1);
  }
  return(0);
}
int main()
{
	//init
	int semid;
	if ((semid=semget(IPC_PRIVATE,1,SEMPERM|IPC_CREAT|IPC_EXCL))==-1)
  	{
		printf("error");	
  	}		
	int status; 
    //设置信号量的值
	semun arg;
    arg.val=10;                                        //信号量的初值
    status=semctl(semid,0,SETVAL,arg);
	printf("semid is %d\n",semid);
	
	int i ;
	for(i=0;i<15;i++)
	{
//		printf("start to fork");
		pid_t pid;
		pid= fork();
		if(pid==0)
		{
//			printf("[%d]start ...\n",getpid());
			//child
			sleep(i);
			p(semid);
			printf("[%d]get sem....",getpid());
			printf("start to work...\n");
			sleep(16-i);
			v(semid);
			exit(0);
		}
		else if(pid==-1)
		{
			printf("fork error\n");
		}
		else
		{
//			printf("新进程%d\n",pid);
		}
	}
	printf("父进程完成\n");
	while(1)
	{
		sleep(1);
		printf("sem 信号量值 是%d\n",semctl(semid, 0, GETVAL));
	}
	return 0;
}
