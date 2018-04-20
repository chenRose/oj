#ifndef _COMMON_H_
#define _COMMON_H_
#include "log.h"
#include "judger.h"
#include "iniparser.h"
#include<stdlib.h>
#include<sys/user.h>
#include<sys/signal.h>
#include<fcntl.h>
#include<stdio.h>
#include<sys/ptrace.h>
#include<sys/resource.h>
#include<sys/reg.h>
#include<sys/sem.h>
#include<sys/signal.h>
/*
 * 运行结果：
 */

typedef enum
{
	OJ_AC,				//ACCEPT		0
	OJ_CE,				//编译错误		1
	OJ_RE,				//运行时出错	2
	OJ_TLE,				//时间超限		3
	OJ_OLE,				//输出超限		4
	OJ_MLE,				//内存超限		5
	OJ_WA,				//答案错误		6
	OJ_PE,				//格式错误		7
	OJ_RF				//危险操作		8
}OJ_STATUS;	
#define OJ_AC 0 		//ACCEPT
#define OJ_CE 1			//编译错误:compile error
#define OJ_RE 2			//运行错误
#define OJ_OT 3			//时间超过限制 
#define OJ_OM 4			//内存超过限制 
#define OJ_OO 5			//输出超过限制
#define OJ_WA 6			//答案错误
char *str_oj_status [] = { "Accept",
					"compile error",
					"running error",
					"time limit error",
					"open limit error",
					"memory limit error",
					"wrong answer",
					"format error",
					"forbidden behavior"
					};

int compare_files(const char *file1,const char *file2);
void printf_oj_status(OJ_STATUS oj_status)
{
	switch(oj_status)
	{
		case OJ_AC:
			log_notice("Accept");
		break;
		case OJ_CE:
			log_error("compile error");
		break;
		case OJ_RE:
            log_error("running error");
        break;
		case OJ_TLE:
            log_error("time limit error");
        break;
		case OJ_OLE:
            log_error("open limit error");
        break;
		case OJ_MLE:
            log_error("memory limit error");
        break;
        case OJ_WA:
            log_error("wrong answer");
        break;
        case OJ_PE:
            log_error("format error");
        break;
        case OJ_RF:
            log_error("forbidden behavior");
        break;
	}
}
typedef struct HANDLER_LOAD_INFO_T
{
	char name[10];
	short total;			
	short pending;
}handler_load_info_t;


/*
 * report_type：1:register
 *              0:autoupload
 */

#define MSG_HANDLER_NUM 10
typedef struct CLIENT_MSG_T
{
	int report_type;					//new task (from php) or report of judger
	int info_num;
	int update;						
	handler_load_info_t load[MSG_HANDLER_NUM];		//结构中只有10个handler_load_info_t结构的容量，而info_num为实际个数，如果info_num超过10，说明还可以再读取info_num-10个handler_load_info结构
}client_msg_t;	



/*信号量操作*/
typedef union _semun {
  int val;
  struct semid_ds *buf;
  ushort *array;
} semun;

int sem_v(int semid)
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

int sem_p(int semid)
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
//设置单个信号量的值：
int sem_set(int semid,int value)
{
	int status;
	semun arg;
	arg.val=value;
	status = semctl(semid,0,SETVAL,arg);
	return status;
}
int sem_get(int semid)
{
	return semctl(semid,0,GETVAL);
}
/*获取消息队列中堆积的消息数量*/
int msg_get_qnum(int msgid)
{
	struct msqid_ds buf;
	if(msgctl(msgid,IPC_STAT,&buf)==-1)
	{
		perror("msgctl");
		log_error_s("rB","获取消息队列堆积消息错误[%d]",msgid);
		return -1;
	}
	return buf.msg_qnum;
	
}
//运行命令：使用的是system
int execute_cmd(const char * fmt, ...) {
    char cmd[1024];
    int ret = 0;
    va_list ap;

    va_start(ap, fmt);
    vsprintf(cmd, fmt, ap);
    ret = system(cmd);
    va_end(ap);
    return ret;
}
FILE* Popen(const char *fmt,...)
{
	char cmd[1024];

	va_list ap;

	va_start(ap, fmt);
	vsprintf(cmd, fmt, ap);
	va_end(ap);
	//返回的FILE* 指针，可以用来读取cmd的输出
	return popen(cmd,"r");
}
/*比较*/
int compare_files(const char *file1,const char *file2)
{
	char cmd[1024];
	FILE* file = Popen("diff %s %s",file1,file2);
	if(file==NULL)
	{
		perror("popen");
		log_error("popen error %s && %s",file1,file2);
	}
	
	log_notice("compare...%s && %s",file1,file2);
	char line[1024];
	if(fgets(line,1024,file)==NULL)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

#endif
