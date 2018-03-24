#include<stdio.h>
#include<string.h>
#include<dlfcn.h>
#include "judger.h"
#include "iniparser.h"
#include "hiredis/hiredis.h"
#include "log.h"
#include "common.h"
#include<sys/msg.h>
#define CONNECT_INI "/home/oj/etc/judger_connect.ini"
#define HANDLER_INI "/home/oj/etc/judger_handler.ini"

dictionary *conn_ini;
dictionary *handler_ini;

redisContext *redis_conn;
judger_conf_t *conn_conf;
loader_interface_t *loader;

int initial_connect_redis(dictionary *conn_ini)
{
    conn_conf->redis_ip=iniparser_getstring(conn_ini,"redis:host",NULL);
    conn_conf->redis_port=iniparser_getint(conn_ini,"redis:port",0);
    conn_conf->redis_passwd=iniparser_getstring(conn_ini,"redis:passwd",NULL);   
	redis_conn=redisConnect(conn_conf->redis_ip,conn_conf->redis_port);
	if(redis_conn!=NULL && redis_conn->err)
	{
		log_error("connect redis error:%s",redis_conn->errstr);
		return -1;
	}
	else
	{
		log_info("connect to redis [%s:%d] successful!",conn_conf->redis_ip,conn_conf->redis_port);
	}
	redisReply *reply;
	reply = redisCommand(redis_conn,"AUTH %s",conn_conf->redis_passwd);
	if (reply->type == REDIS_REPLY_ERROR) {
		log_error("Authentication failed\n");
		freeReplyObject(reply);
		return -1;
	}
	freeReplyObject(reply);	
	return 0;
}
int initial_connect_core(dictionary *conn_ini)
{
	//todo：用于与core进行连接，这一部分先放着，等之后在编写core时一起编写。）
	return 0;
}
void finish()
{
	
	iniparser_freedict(conn_ini);
	iniparser_freedict(handler_ini);
	redisFree(redis_conn);
	free(conn_conf);
}
int init_connect()
{
	int iRet;
//先读取connecct配置文件：用于连接core与redis
	conn_ini=iniparser_load(CONNECT_INI);

    conn_conf=malloc(sizeof(judger_conf_t));
    memset(conn_conf,0,sizeof(judger_conf_t));

	iRet=initial_connect_redis(conn_ini);
	if(iRet==-1)
	{
		log_error("error to connect  redis %s[%d]\n",conn_conf->redis_ip,conn_conf->redis_port);
		return -1;
	}
    initial_connect_core(conn_ini);
	if(iRet==-1)
    {
        log_error("error to connect  core %s[%d]\n",conn_conf->core_ip,conn_conf->core_port);
        return -1;
    }
	return 0;
}

int  handler_loader()
{
	loader = malloc(sizeof(loader_interface_t));
	memset(loader,0,sizeof(loader_interface_t));
	
	handler_ini=iniparser_load(HANDLER_INI);
	
	loader->dir=iniparser_getstring(handler_ini,"global:dir",NULL);
	loader->num=iniparser_getint(handler_ini,"global:num",0);
	int	msg_id=msgget(ftok("/home/oj/src",1204),IPC_CREAT);
	
	loader->head=NULL;
	loader->tail=NULL;

	printf("\nloader->num is %d",loader->num);
	printf("msg id is %d\n",msg_id);
	printf(loader->dir);	
	printf("\n");
    int i ;
    for(i=1;i<=loader->num;i++)
    {
		
		handler_interface_t *handler = malloc(sizeof(handler_interface_t));     //生成一个新的handler接口，保存相应的信息
		memset(handler,0,sizeof(handler_interface_t));
		
		default_load_ini(loader,handler,handler_ini,i,msg_id);
		//set function pointer 
		void *module = dlopen(handler->module_path,RTLD_LAZY);
		if(!module)
		{
			log_error("open %s error,no such file? ",handler->module_path);
		}
		else			
		{	
			//打开模块，用模块中的函数地址来初始化相应的handler结构
			handler->initial=dlsym(module,"initial");
			handler->work=dlsym(module,"work");
			handler->reload=dlsym(module,"reload");
		}
		//insert to list

		if(loader->head==NULL && loader->tail==NULL)
		{
			loader->head=loader->tail=handler;
		}
		else
		{
			handler->prev = loader->tail;
			loader->tail->next=handler;
			loader->tail = handler;
		}
	}
	printf("已经读取完配置文件.....\n");
	return 0;
}

int start_all_handler()
{
	handler_interface_t *handler = loader->head;
	while(handler!=NULL)
	{
		if(handler->initial==NULL)
		{
			handler=handler->next;
			continue;
		}
		handler->initial(handler,handler_ini);			//可能有一些handler特有的配置项，此initial用于读取这些配置项（保存在handler->info指针中）
		int i ;
		pid_t pid;
		for(i=0;i<handler->n_worker;i++)
		{
			//创建相应的worker
			pid = fork();
			if(pid==0)	//child
			{
				if(handler->work!=NULL)
				{
					handler->work();
				}
				else
				{
					log_error("handler[%s] don't work,please check whether the work function has been implemented in %s",handler->name,handler->module_path);
					exit(0);
				}
				exit(0);
			}
			else
			{
				handler->pids[i]=pid;		
			}
			
		}
		handler=handler->next;
	}
	printf("all handler start ...\n");

	
	sleep(1);
	mymsg_t  *msg = malloc(sizeof(mymsg_t));
	msg->mtype=1;
	strncpy(msg->cmd,"chenshuyu",sizeof("chenshuyu"));
	msgsnd(loader->head->msg_id,msg,sizeof("chenshuyu"),0);
	sleep(1);
	strncpy(msg->cmd,"fangmengting",sizeof("fangmengting"));
	msgsnd(loader->head->msg_id,msg,sizeof("fangmengting"),0);
    sleep(1);
    strncpy(msg->cmd,"fangmengting",sizeof("fangmengting"));
    msgsnd(loader->head->msg_id,msg,sizeof("fangmengting"),0);
    sleep(1);
    strncpy(msg->cmd,"fangmengting",sizeof("fangmengting"));
    msgsnd(loader->head->msg_id,msg,sizeof("fangmengting"),0);
    sleep(1);
    strncpy(msg->cmd,"fangmengting",sizeof("fangmengting"));
    msgsnd(loader->head->msg_id,msg,sizeof("fangmengting"),0);
    sleep(1);
    strncpy(msg->cmd,"fangmengting",sizeof("fangmengting"));
    msgsnd(loader->head->msg_id,msg,sizeof("fangmengting"),0);
    sleep(1);
    strncpy(msg->cmd,"fangmengting",sizeof("fangmengting"));
    msgsnd(loader->head->msg_id,msg,sizeof("fangmengting"),0);
}
void  main()
{
	log_initial("judger[1]",LOG_PID,LOG_USER,0);
	init_connect();
	handler_loader();
	start_all_handler();
	sleep(20000);
	finish();	
}

