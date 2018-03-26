
/*
 * chenshuyu 
 * csy199612@gmail.com
 * 
*/
#include<stdio.h>
#include<string.h>
#include<dlfcn.h>
#include "judger.h"
#include "iniparser.h"
#include "hiredis/hiredis.h"
#include "log.h"
#include <sys/socket.h>
#include<netinet/in.h>
#include "common.h"
#include<sys/msg.h>
#define CONNECT_INI "/home/oj/etc/judger_connect.ini"
#define HANDLER_INI "/home/oj/etc/judger_handler.ini"

//使用iniparser库来读取配置文件，dictionary结构定义在iniparser.h中
//conn_ini：包含redis和core的连接信息：/home/oj/etc/judger_connect.ini
dictionary *conn_ini;
//handler_ini：根据/home/oj/etc/judger_handler.ini中读取handler的启用、实例个数等
dictionary *handler_ini;

//与redis的连接上下文
redisContext *redis_conn;
//与core进行socket连接的fd
int core_sock_fd;
//保存连接（core+redis)配置信息
judger_conf_t *conn_conf;
//保存handler的信息链表
loader_interface_t *loader;

/*
 * 函数作用：初始化redis连接
 * 
 */ 
int initial_connect_redis(dictionary *conn_ini)
{
	//从dictionary结构中读取出redis数据库的设置：ip、port和密码
    conn_conf->redis_ip=iniparser_getstring(conn_ini,"redis:host",NULL);
    conn_conf->redis_port=iniparser_getint(conn_ini,"redis:port",0);
    conn_conf->redis_passwd=iniparser_getstring(conn_ini,"redis:passwd",NULL);   
	//连接
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
	//使用密码认证
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

/*
 * 函数作用：读取配置文件中的core的host/port，建立socket连接
*/
int initial_connect_core(dictionary *conn_ini)
{
	return 0;
	//读取配置文件中core的host和port
	conn_conf->core_host = iniparser_getstring(conn_ini,"core:host",NULL);
	conn_conf->core_port = iniparser_getint(conn_ini,"core:port",0);
	if(conn_conf->core_host==NULL || conn_conf->core_port==0)
	{
		log_error("error core config ,please check judger_connect.ini");
	}
	log_info("connecting to core :[%s:%d]",conn_conf->core_host,conn_conf->core_port);
	//创建socket
	core_sock_fd = socket(AF_INET,SOCK_STREAM,0);
	//构建服务器地址
	struct sockaddr_in servaddr;
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(conn_conf->core_port);
	inet_pton(AF_INET,conn_conf->core_host,&servaddr.sin_addr);
	//进行连接
	if(connect(core_sock_fd,(struct sockaddr*)&servaddr,sizeof(servaddr))==-1)
	{
		log_error("connect error ");
		return -1;
	}
	
	log_info("connect to core :[%s:%d] successful!,socket fd is %d",conn_conf->core_host,conn_conf->core_port,core_sock_fd);
	
	return 0;
}
void finish()
{
	
	iniparser_freedict(conn_ini);
	iniparser_freedict(handler_ini);
	redisFree(redis_conn);
	free(conn_conf);
}
/*
 * 函数作用：使用iniparser库读取配置文件，建立相应的dictionary.主要工作为：
 * 调用函数initial_connect_redis和initial_connect_core两个函数来初始化redis和core的连接
 */
int init_connect()
{
	int iRet;
//先读取connecct配置文件：用于连接core与redis
	conn_ini=iniparser_load(CONNECT_INI);
	//
    conn_conf=malloc(sizeof(judger_conf_t));
    memset(conn_conf,0,sizeof(judger_conf_t));

	iRet=initial_connect_redis(conn_ini);
	if(iRet==-1)
	{
		log_error("error to connect  redis %s[%d]",conn_conf->redis_ip,conn_conf->redis_port);
		return -1;
	}
    iRet = initial_connect_core(conn_ini);
	if(iRet==-1)
    {
        log_error("error to connect  core %s[%d]",conn_conf->core_host,conn_conf->core_port);
        return -1;
    }
	return 0;
}

 void display_handler(loader_interface_t *loader)
 {
	if(loader)
	{
		printf("there are %d handler had been read \n",loader->num);
		printf("-------------------------------------\n");
		handler_interface_t *handler = loader->head;
		while(handler)
		{
			printf("name :\t\t\t\t%s\n",handler->name);
			printf("num of instance:\t\t\t\t%d\n",handler->n_worker);
			printf("module path:\t\t\t\t%s\n",handler->module_path);
			printf("msg_id is \t\t\t\t%d\n",handler->msg_id);
			printf("-------------------------------------\n");	
			handler=handler->next;
		}
		
	}
 }
 /*
 * 函数作用：从dictionary结构中读取相应的handler设置，初始化相应的handler，并把它们组织成为链表（loader)
 */
int  handler_loader()
{
	//分配handler链表结构空间
	loader = malloc(sizeof(loader_interface_t));
	memset(loader,0,sizeof(loader_interface_t));
	//iniparser读取配置文件
	handler_ini=iniparser_load(HANDLER_INI);
	
	loader->dir=iniparser_getstring(handler_ini,"global:dir",NULL);
	loader->num=iniparser_getint(handler_ini,"global:num",0);
	//设置一个IPC消息队列，但后续应该会改成一个handler有专门的消息队列 
	int	msg_id;
	
	loader->head=NULL;
	loader->tail=NULL;

	printf("\nthere are %d handler in configure file \n",loader->num);
	printf("default dir is :%s\n",loader->dir);	
	int i ;
    for(i=1;i<=loader->num;i++)
    {
		
		handler_interface_t *handler = malloc(sizeof(handler_interface_t));     //生成一个新的handler接口，保存相应的信息
		memset(handler,0,sizeof(handler_interface_t));
		/*load ini
		 *loader:	handler list
		 * handler: handler_interface_t
		 * handler_ini：dictionary
		 * i:sequence num of handler 
	 	 */		
		default_load_ini(loader,handler,handler_ini,i);
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
	display_handler(loader);
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
					exit(1);
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
	log_info("[judger1]start to initial...");
	log_info("start to initial connect [redis & core]");
	if(init_connect()==-1)
	{
		log_error("error to initial connect ,exit");
		exit(1);
	}
	handler_loader();
	start_all_handler();
	sleep(20000);
	finish();	
}

