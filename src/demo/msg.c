#include<stdio.h>
#include<stdlib.h>
#include<sys/sem.h>
#include<sys/msg.h>
#define SEMPERM 0600

typedef union _semun {
  int val;
  struct semid_ds *buf;
  ushort *array;
} semun;


int main()
{
	int id = msgget(12345,IPC_CREAT);
	printf("first id %d\n",id);
	
	struct msqid_ds buf;
	msgctl(id,IPC_STAT,&buf);
	return buf.msg_qnum;	
		
		
	return 0;
}
