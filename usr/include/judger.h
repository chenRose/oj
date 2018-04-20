#ifndef _JUDGER_H_
#define _JUDGER_H_

#include "log.h"
#include "iniparser.h"
#include<sys/msg.h>
#include<sys/sem.h>
typedef struct {
	/*core*/
	const char *host_name;
	const char *core_host;
	int  core_port;
	/*redis*/
	const char *redis_ip;
	int  redis_port;
	const char *redis_passwd;
	const char *redis_list;
	/* mysql */
	const char *mysql_ip;
	int mysql_port;
	const char *mysql_user;
	const char *mysql_passwd;
	const char *mysql_db;
}judger_conf_t;

struct HANDLER_INTERFACE_T ;
typedef int (*PF_work)(void *);
typedef int (*PF_reload)(struct HANDLER_INTERFACE_T *,dictionary*);
typedef int (*PF_initial)(struct HANDLER_INTERFACE_T* ,dictionary*);
typedef int (*PF_finish)();
typedef int (*PF_get_capacity)();
typedef int (*PF_get_available)();

typedef struct TASK_T
{
	long  match_num;
	long  ques_num;
	long  user_num;
	const char * text;
	long  commit_num;
	int code_len;
	char dir[1024];
	char source[20];
	char name[20]; 
	char task_type[10];
	char  result[20];
	int score;
	int use_time;
	int use_memory;

	
}task_t;
#define TEXT "text"
#define MATCH_NUM "match_num"
#define USER_NUM "user_num"
#define QUES_NUM "ques_num"
#define COMMIT_NUM "commit_num"
#define TASK_TYPE "task_type"

typedef struct HANDLER_INTERFACE_T{	
//	typedef int (*PF_initial)(struct HANDLER_INTERFACE_T *,dictionary* );

    const char *name;       	//handler name
    int seq_num;               	//sequence number
    int workable;          	 	//is enable?
    const char *module_dir;     //.so path
    const char *module_name;	//.so name
	char module_path[1024];		//.so absolution path
	int n_worker;				//worker num
	struct HANDLER_INTERFACE_T *prev;		//list
	struct HANDLER_INTERFACE_T *next;		
	void *info;					//eg ：custom conf
	char suffix[10];

	pid_t pids[64];
	/* api接口函数指针 */
	PF_initial handler_initial;			//read conf && check the compiler exist
	PF_work handler_work;				//worker 
	PF_reload handler_reload;			//reload conf
	PF_finish handler_finish;				//release resource
	PF_get_available handler_get_available;
	PF_get_capacity handler_get_capacity;


	int msg_id;					//IPC mq_id
	int sem_id;					//IPC sem_id
	int master_msg_id;			
}handler_interface_t;

typedef struct COMMIT_RESULT_T
{
	
}commit_result_t;
//file judger_handler.ini 
//list为模块（handler）组织而成的链表
//其它信息为global 节的信息
typedef struct {				
	const char *dir;				//
	int num;
	int n_workers;
	int msg_id;
	int sem_id;
	int master_msg_id;
	handler_interface_t *head;
	handler_interface_t *tail;
}loader_interface_t;

typedef struct {
	long mtype;
	task_t task;

}mymsg_t;

int default_load_ini(loader_interface_t *loader,handler_interface_t *handler,dictionary *handler_ini,int seqnum)
{
		handler->seq_num=seqnum;


		const char *sec_name = iniparser_getsecname(handler_ini,seqnum);
		char item [1024] = {0};
        sprintf(item,"%s:",sec_name);
        int index = strlen(item);
        sprintf(item+index,"name");
        handler->name = iniparser_getstring(handler_ini,item,NULL);


        sprintf(item+index,"worker");
        handler->n_worker = iniparser_getint(handler_ini,item,0);

		
        sprintf(item+index,"enable");
        handler->workable= strcasecmp(iniparser_getstring(handler_ini,item,NULL),"true")?0:1;


        sprintf(item+index,"path");
        const char *item_path = iniparser_getstring(handler_ini,item,NULL);
	
		sprintf(item+index,"suffix");
		const char *item_suffix = iniparser_getstring(handler_ini,item,NULL);
		if(item_suffix!=NULL)
				strncpy(handler->suffix,item_suffix,10);
		else
				handler->suffix[0]='\0';
	
        if (!item_path)
        {
            log_error("error in judger_handler.ini:[%s:path],please check it  and try again",sec_name);
            return -1;
        }
        if(item_path[0]=='/')
        {
            //绝对路径
        }
		else
        {
            handler->module_dir = loader->dir;
            handler->module_name = item_path;
            snprintf(handler->module_path,sizeof(handler->module_path),"%s/%s",handler->module_dir,handler->module_name);
        }
		int msg_id = msgget(ftok(handler->module_path,handler->n_worker),IPC_CREAT|IPC_EXCL);
		if(msg_id==-1)
		{
			//消息队列已经存在，先清空
			printf("消息队列已经存在");
			msg_id = msgget(ftok(handler->module_path,handler->n_worker),IPC_CREAT);
			printf(":%d\t",msg_id);
			//使用IPC_RMID选项删除队列 
			if(msgctl(msg_id,IPC_RMID,NULL)==-1)
			{
				perror("msgctl");
				log_error_s("rB","删除失败");
				
			}
			//重新创建一个新的消息队列 
			msg_id = msgget(ftok(handler->module_path,handler->n_worker),IPC_CREAT|IPC_EXCL);
			printf("已经删除...，重新创建%d\n",msg_id);		
			
		}
		handler->msg_id = msg_id;
		int sem_id;
		sem_id = semget(ftok(handler->module_path,handler->n_worker),1,IPC_CREAT|IPC_EXCL);
		if(sem_id==-1)
		{
			printf("信号量已经存在");
	        sem_id = semget(ftok(handler->module_path,handler->n_worker),1,IPC_CREAT);
			printf(":%d\t",sem_id);
			semctl(sem_id,0,IPC_RMID);
			sem_id = semget(ftok(handler->module_path,handler->n_worker),1,IPC_CREAT|IPC_EXCL);
			printf("已经删除...，重新创建%d\n",sem_id);
		}
		handler->sem_id=sem_id;
		sem_set(handler->sem_id,handler->n_worker);
		printf("...sem id ：%d(%d)\n",sem_get(handler->sem_id),handler->sem_id);
}
#endif
