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
#include "sys/errno.h"
#include "common.h"
#include<sys/types.h>  
#include<sys/stat.h>  
#include<sys/msg.h>
#include <unistd.h>
#include "mysql/mysql.h"
#include <sys/types.h>
#include<signal.h>
#include<sys/sem.h>
#define CONNECT_INI "/home/oj/etc/judger_connect.ini"
#define HANDLER_INI "/home/oj/etc/judger_handler.ini"
#include<fcntl.h>
#define JUDGER_NAME "JUDGER1"
//使用iniparser库来读取配置文件，dictionary结构定义在iniparser.h中
//conn_ini：包含redis和core的连接信息：/home/oj/etc/judger_connect.ini
dictionary *conn_ini;
//handler_ini：根据/home/oj/etc/judger_handler.ini中读取handler的启用、实例个数等
dictionary *handler_ini;

#define SOURCE_DIR "/data/oj"
//与redis的连接上下文
redisContext *redis_conn;
//与core进行socket连接的fd
int core_sock_fd;
//与mysql的连接上下文
MYSQL *mysql_conn;
//保存连接（core+redis)配置信息
judger_conf_t *conn_conf;
//保存handler的信息链表
loader_interface_t *loader;

/*
 * 函数作用：根据task_t结构中的比赛号、任务号、用户号、提交次数，获取一个保存相应代码的路径
 * 路径名规则://todo
 * 最终的文件名会有根据相应的接口生成随机名
 * 生成的路径名会保存在task中 *
 *
 */
int get_source_file_path(task_t* task)
{
	//保存生成的路径
    char path_buf[1024];
	char old_dir[1024];
	//根据{SOURCE_DIR}/match_num/user_num/ques_num/commit_num.  来创建相应代码保存的目录
    snprintf(path_buf,1024,"%s/%ld/%ld/%ld",SOURCE_DIR,task->match_num,task->ques_num,task->user_num);
	log_error_s("rBF","%ld-%ld",task->match_num,task->ques_num);
	int dir_fd = open(path_buf,O_RDWR);
	
	getcwd(old_dir,sizeof(old_dir));
	if(dir_fd==-1)
	{
		//没有相对应的目录名，应该创建相对应的目录
	    char new_dir[1024];

		///进入根目录
	    chdir(SOURCE_DIR);
	    //进入第一层子目录:match_num
	    snprintf(new_dir,1024,"%ld",task->match_num);
   		if(chdir(new_dir)==-1)		//没有这个目录文件
	    {
       		mkdir(new_dir,0755);
       		chdir(new_dir);
    	}		
		//进入第二层子目录：user_num
		snprintf(new_dir,1024,"%ld",task->ques_num);
		if(chdir(new_dir)==-1)
		{
			mkdir(new_dir,0755);
			chdir(new_dir);
		}	
		//进入第三层子目录：ques_num
		snprintf(new_dir,1024,"%ld",task->user_num);
		if(chdir(new_dir)==-1)
		{
			mkdir(new_dir,0755);
			chdir(new_dir);
		}
	}
	//现在已经成功创建了相应的目录，目录名保存在path_buf。将它保存到task结构中。
	strncpy(task->dir,path_buf,sizeof(task->dir));
	//并且，现在的work—dir正是此目录
	//使用mkstemp函数来创建一个带随机性的文件名，并打开、返回它的fd
	snprintf(task->name,sizeof(task->name),"%04dXXXXXX",task->commit_num);
	int fd = mkstemp(task->name);
	if(fd>0)
	{
		
		log_info("新任务，代码文件保存位置为:%s/%s\n",task->dir,task->name);
		//修改回原来的工作目录
		chdir(old_dir);
		return fd;
	}
	chdir(old_dir);
	return -1;

}
int save_file_from_redis(const char *key,const char *file,mode_t mode)
{
	redisReply *reply;
	char command [1024];
	snprintf(command,1024,"get %s",key);
	
	int fd = open(file,O_CREAT|O_RDWR|O_EXCL,mode);
	if(fd==-1)
	{
		log_error("create file %s error,file exist?",file);
	}
	reply = redisCommand(redis_conn,command);
	if(reply==NULL)
	{
		log_error("run redis command %s error",command);
		return -1;
	}
	if(reply->type==REDIS_REPLY_STRING)
	{
		log_notice("write ... %d\n",reply->len);
		write(fd,reply->str,reply->len);
	}
	else if (reply->type==REDIS_REPLY_NIL)
	{
		log_error("error...key %s not found from redis \n",key);
		return -1;
	}
	
}
/*
 * 函数作用：从redis数据库中读取一个新的任务
 * 参数：const char *key，此为记录的key值
 *
 */
int read_task_from_redis(const char *key,task_t *task)
{
	redisReply *reply;
	int i;
	//根据key发送命令
	char command [1024];
	snprintf(command,1024,"HGETALL %s",key);
	reply = redisCommand(redis_conn,command);
	//如果从redis返回的reply是一个数组
	log_error("---------------------------------------------------------------");
	if(reply->type==REDIS_REPLY_ARRAY)	
	{	
		//需要注意的是，如果给定的key对应的hash是空的，返回的也是一个REDIS_REPLY_ARRAY，但它的elements是0
		if(reply->elements==0)
		{
			log_error("未找到相应的key： %s",key);
			return -1;
		}
		for(i=0;i<reply->elements;i+=2)	//遍历element数组
		{
			redisReply *elem = reply->element[i];
			//取得hash中的key对应的redisReply结构
			if(elem->type==REDIS_REPLY_STRING)
			{					
				if(strncmp(elem->str,TEXT,elem->len)==0)//text key，对应的value即是代码
				{
					task->text = reply->element[i+1]->str;				
					task->code_len = reply->element[i+1]->len;
				}
				else if(strncmp(elem->str,MATCH_NUM,elem->len)==0)//match_num，对应的value是比赛号
				{
					log_info("match_num:%s\n",reply->element[i+1]->str);
					task->match_num = atol(reply->element[i+1]->str);
				}
				else if(strncmp(elem->str,QUES_NUM,elem->len)==0)
				{
					task->ques_num = atol(reply->element[i+1]->str);
				}
				else if(strncmp(elem->str,COMMIT_NUM,elem->len)==0)
				{
					task->commit_num = atol(reply->element[i+1]->str);
				}
				else if(strncmp(elem->str,USER_NUM,elem->len)==0)
				{
					task->user_num = atol(reply->element[i+1]->str);
				}
				else if(strncmp(elem->str,TASK_TYPE,elem->len)==0)
				{
					//任务类型，将决定它会被发送到哪个handler来处理
					strncpy(task->task_type,reply->element[i+1]->str,10);	
					log_notice("type:%s\n",reply->element[i+1]->str);
				}
			}
			else
			{
				//hash中的key应该都是string类型了
				log_error("read a hash in error style\n");
				return -1;
			}
		}
	} 
	else
	{
		//根据key读取得到值不是一个hash，理应也是一个error
		log_error("error key...ignore...");
		return -1;
	}
	//调用get_source_file_path，此函数会根据task结构，生成一个对应的文件，打开并返回它的fd
	int fd = get_source_file_path(task);
	//将text部分打印到相应的代码文件中
	write(fd,task->text,task->code_len);
	close(fd);
	
	log_error("---------------------------------------------------------------");
	freeReplyObject(reply);//注意，此时reply被释放，后续的task->text就不可以使用了
	task->text =NULL;
	return 0;
}                                   

/*
 * 函数作用：初始化redis连接
 * 
 */ 
int initial_connect_redis(dictionary *conn_ini)
{
	log_info("start to connect redis...");
	
	//从dictionary结构中读取出redis数据库的设置：ip、port和密码
    conn_conf->redis_ip=iniparser_getstring(conn_ini,"redis:host",NULL);
    conn_conf->redis_port=iniparser_getint(conn_ini,"redis:port",0);
    conn_conf->redis_passwd=iniparser_getstring(conn_ini,"redis:passwd",NULL);   
	//连接
	log_info("\t[server-ip:%s,server-port:%d]\n",conn_conf->redis_ip,conn_conf->redis_port);
	redis_conn=redisConnect(conn_conf->redis_ip,conn_conf->redis_port);
	if(redis_conn!=NULL && redis_conn->err)
	{
		log_error("[redis] connect  error:%s",redis_conn->errstr);
		return -1;
	}
	else
	{
		log_notice("connect to redis [%s:%d] !\n",conn_conf->redis_ip,conn_conf->redis_port);
	}
	//使用密码认证
	redisReply *reply;
	log_info("AUTH password...\n");
	reply = redisCommand(redis_conn,"AUTH %s",conn_conf->redis_passwd);
	if (reply->type == REDIS_REPLY_ERROR) {
		log_error("Authentication failed\n");
		freeReplyObject(reply);
		return -1;
	}
	log_notice("Authentication successful!\n");
	log_notice("connect to redis [%s:%d] successful!\n",conn_conf->redis_ip,conn_conf->redis_port);
	freeReplyObject(reply);	
	return 0;
}

int initial_connect_mysql(dictionary *conn_ini)
{
	log_info("start to connect mysql...");
	//从dictionary结构中读取mysql的设置：ip、port、user、passwd
	conn_conf->mysql_ip=iniparser_getstring(conn_ini,"mysql:host",NULL);
    conn_conf->mysql_port=iniparser_getint(conn_ini,"mysql:port",0);
    conn_conf->mysql_passwd=iniparser_getstring(conn_ini,"mysql:passwd",NULL);   	
    conn_conf->mysql_db=iniparser_getstring(conn_ini,"mysql:dbname",NULL);
    conn_conf->mysql_user=iniparser_getstring(conn_ini,"mysql:user",NULL);  
	//调用mysql中的函数来建立连接
	log_info("\t[server-ip:%s,server-port:%d,[DB:%s][user:%s]]\n",conn_conf->mysql_ip,conn_conf->mysql_port,conn_conf->mysql_db,conn_conf->mysql_user);
	//初始化mysql库
	log_info("initial MYSQL library...\n");
	if (mysql_library_init(0, NULL, NULL))
    {
        log_error("could not initialize MySQL library\n");
		return -1;
    }
	//初始化MYSQL结构
	mysql_conn = malloc(sizeof(MYSQL));
	mysql_init(mysql_conn);
	int timeout = 30;	
	mysql_options(mysql_conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
	MYSQL *ret = mysql_real_connect(mysql_conn,
									conn_conf->mysql_ip,			//ip
									conn_conf->mysql_user,			//用户名
									conn_conf->mysql_passwd,		//密码
									conn_conf->mysql_db,			//数据库
									conn_conf->mysql_port,			//port:default	
									NULL,							//unix socket
									0);
	if(ret == NULL)
	{
		log_error("[Mysql]connect error:%s",mysql_error(mysql_conn));
		 mysql_close(mysql_conn);
         mysql_library_end();
		 return -1;
	}
	log_notice("connect to Mysql successful! [%s:%d] \n",conn_conf->redis_ip,conn_conf->redis_port);
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
	log_info("connecting to core :[%s:%d]\n",conn_conf->core_host,conn_conf->core_port);
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
	
	log_info("connect to core :[%s:%d] successful!,socket fd is %d\n",conn_conf->core_host,conn_conf->core_port,core_sock_fd);
	
	return 0;
}

void finish()
{
	//清除IPC
	msgctl(loader->msg_id,IPC_RMID,NULL);
	semctl(loader->sem_id,0,IPC_RMID);
	iniparser_freedict(conn_ini);
	iniparser_freedict(handler_ini);
	
	redisFree(redis_conn);
	printf("hello...\n");
	if(conn_conf!=NULL)
	{
		free(conn_conf);
	}
	exit(0);
	
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
	conn_conf->host_name =iniparser_getstring(conn_ini,"core:name",NULL);
	conn_conf->redis_list =iniparser_getstring(conn_ini,"redis:list",NULL);
	iRet=initial_connect_redis(conn_ini);
	if(iRet==-1)
	{
		log_error("error to connect  redis %s[%d]",conn_conf->redis_ip,conn_conf->redis_port);
		return -1;
	}
	iRet=initial_connect_mysql(conn_ini);
	if(iRet ==-1)
	{
		log_error("error to connect  mysql %s[%d]",conn_conf->redis_ip,conn_conf->redis_port);
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
		log_notice_s("gB","there are %d handler had been read \n",loader->num);
		log_notice_s("rB","--------------------------------------------------------------------------------------\n");
		handler_interface_t *handler = loader->head;
		while(handler)
		{
			log_info("name :\t\t\t\t\t%s\n",handler->name);
			log_info("num of instance:\t\t\t%d\n",handler->n_worker);
			log_info("module path:\t\t\t\t%s\n",handler->module_path);
			log_info("msg_id is \t\t\t\t%d\n",handler->msg_id);
			log_info("sem_id is \t\t\t\t%d\n",handler->sem_id);
			log_notice_s("rB","--------------------------------------------------------------------------------------\n");	
			handler=handler->next;
		}
		
	}
 }
 /*
 * 函数作用：从dictionary结构中读取相应的handler设置，初始化相应的handler，并把它们组织成为链表（loader)
 */
int handler_loader()
{
	//分配handler链表结构空间
	loader = malloc(sizeof(loader_interface_t));
	memset(loader,0,sizeof(loader_interface_t));
	//iniparser读取配置文件
	handler_ini=iniparser_load(HANDLER_INI);
	
	loader->dir=iniparser_getstring(handler_ini,"global:dir",NULL);
	loader->num=iniparser_getint(handler_ini,"global:num",0);
	loader->n_workers = iniparser_getint(handler_ini,"global:worker",0);
	
	int msg_id,sem_id;
	//这里直接用IPC_PRIVATE，表示它将创建一个新的消息队列
	loader->msg_id = msgget(IPC_PRIVATE,0600|IPC_CREAT|IPC_EXCL);
	loader->sem_id = semget(IPC_PRIVATE,1,0600|IPC_CREAT|IPC_EXCL);
	
	log_notice("公共消息队列id为：%d\n",loader->msg_id);
	log_notice("公共信号量id为：%d\n",loader->sem_id);
	//初始化信号量的值
	sem_set(loader->sem_id,loader->n_workers);	

	//创建一个消息队列，这个消息队列将用于handler的worker完成一个任务时
	//向该消息队列传递一个消息。另外，有一个进程统一监听该队列，来将结果写入mysql
	msg_id== msgget(IPC_PRIVATE,0600|IPC_CREAT);
	if(msg_id==-1)
	{
		perror("msgget");
		log_error("msgget error:in create a master msg");
		return -1;
	}
	loader->master_msg_id =msg_id;
	
	log_notice("master消息队列id为：%d\n",loader->master_msg_id);
	loader->head=NULL;
	loader->tail=NULL;
	
	log_info("开始读取配置文件:%s \n",HANDLER_INI);
	log_info("配置文件中共有 %d 个handler,开始处理... \n",loader->num);
	log_info("共有%d个公共的worker\n",loader->n_workers);
	log_info("配置文件中模块默认目录为 :%s\n",loader->dir);	
	int i ;
	
	execute_cmd("/home/oj/src/handler/make.sh");
	//继续读取配置文件中handler的配置
    for(i=1;i<=loader->num;i++)
    {
		
		handler_interface_t *handler = malloc(sizeof(handler_interface_t));     //生成一个新的handler接口，保存相应的信息
		handler->master_msg_id = loader->master_msg_id;
		memset(handler,0,sizeof(handler_interface_t));
		/* load ini
		 * loader:	handler list
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
			handler->handler_initial=dlsym(module,"handler_initial");
			handler->handler_work=dlsym(module,"handler_work");
			handler->handler_reload=dlsym(module,"handler_reload");			
			handler->handler_get_capacity=dlsym(module,"handler_get_capacity");
			handler->handler_get_available=dlsym(module,"handler_get_available");
			handler->handler_finish=dlsym(module,"handler_finish");
			if(handler->handler_initial==NULL)
			{
				log_notice_s("pB","warning.handler[%s] API [%s] is NULL\n",handler->name,"handler_initial");
			}
			if(handler->handler_work==NULL)
			{
				log_notice_s("pB","warning.handler[%s] API [%s] is NULL\n",handler->name,"handler_work");
			}
			if(handler->handler_reload==NULL)
			{
				log_notice_s("pB","warning.handler[%s] API [%s] is NULL\n",handler->name,"handler_reload");
			}
			if(handler->handler_get_capacity==NULL)
			{
				log_notice_s("pB","warning.handler[%s] API [%s] is NULL\n",handler->name,"handler_get_capacity");
			}
			if(handler->handler_get_available==NULL)
			{
				log_notice_s("pB","warning.handler[%s] API [%s] is NULL\n",handler->name,"handler_get_available");
			}
			if(handler->handler_finish==NULL)
			{
				log_notice_s("pB","warning.handler[%s] API [%s] is NULL\n",handler->name,"handler_finish");
			}	
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
	log_notice("完成配置文件的读取，handler配置如下：\n");
	
	log_notice_s("rB","--------------------------------------------------------------------------------------");
	display_handler(loader);
	log_notice_s("rB","--------------------------------------------------------------------------------------");
	return 0;
}
int task_write_to_mysql(task_t* task)
{
	char cmd  [2048];
	snprintf(cmd,sizeof(cmd),"update submit "  
							 "set submit_use_time=%d,"
									  "submit_use_mem=%d,"
									  "submit_score=%d,"
									  "submit_result=\"%s\"  "
							 "where submit_user_id = %ld and "
									"submit_contest_id = %ld and "
									"submit_question_id = %ld and "
									"submit_user_num = %ld ;",
									  task->use_time,task->use_memory,task->score,task->result,
									  task->user_num,task->match_num,task->ques_num,task->commit_num);
	log_notice("cmd :%s",cmd);
	if (0 != mysql_real_query(mysql_conn, cmd, strlen(cmd)))
	{
		 log_error("query failed: %s\n", mysql_error(mysql_conn));
		 mysql_close(mysql_conn);
		 mysql_library_end();
		 return -1;
	}
				
}
int get_judger_load()
{

    log_info("--------------------------------------------------------------------------------------\n");
	log_notice("common worker load:%d/%d\n",sem_get(loader->sem_id),loader->n_workers);
    log_info("--------------------------------------------------------------------------------------\n");
	handler_interface_t * handler =loader->head;
    log_info("--------------------------------------------------------------------------------------\n");
	while(handler!=NULL)
	{
		log_info("%s worker load:%d/%d\n",handler->name,sem_get(handler->sem_id),handler->n_worker);
		handler=handler->next;
	}
    log_info("--------------------------------------------------------------------------------------\n");
}
void common_worker_work()
{
	log_notice("[common :%d]\tworker wating a task...",getpid());
	while(1)
	{
		//监听相应的消息队列，以读取一个消息
		mymsg_t *msg = malloc(sizeof(mymsg_t));	
		int iRet = msgrcv(loader->msg_id,msg,2048,0,0);
		log_notice("[c:%d]\t get task :%s/%s \n",getpid(),msg->task.dir,msg->task.source);
		//进行一个p操作，对信号量减1，表示当前空闲的进程数量-1
		sem_p(loader->sem_id);
		//开始工作
		

		sem_v(loader->sem_id);	
	}
	
}
int start_all_handler()
{
	//先创建所有的公共worker
	int i;
	pid_t pid;
	for(i=0;i<loader->n_workers;i++)
	{
		pid = fork();
		if(pid==0)
		{
			common_worker_work();	
			//child
			exit(0);
		}	
	}	
	//创建一个进程，监听master_msg_id队列，并将得到的内容写回mysql数据库
	pid = fork();
	if(pid==0)
	{
		while(1)
		{
			log_notice("[%s:%d]\tmaster_msg wating a task...[%d]","master_msg",getpid(),loader->master_msg_id);
			mymsg_t *msg=malloc(sizeof(mymsg_t));	
			int iRet = msgrcv(loader->master_msg_id,msg,2048,0,0);//block until new command
			//将msg的内容保存到task_t结构中
			task_t *task = malloc(sizeof(task_t));
			memcpy(task,&(msg->task),sizeof(task_t));
			log_notice("[master msg worker]get a task:\n"
						"score:\t\t%d\n"
					   "use_time:\t%dms\n"
					   "use_memory:\t%dKB\n"
					   "result:\t\t%s\n",msg->task.score,msg->task.use_time,msg->task.use_memory,msg->task.result);
			task_write_to_mysql(task);
			free(msg);			
		}
		exit(0);
	}	
	
	//根据handler链表，来启动handler
	handler_interface_t *handler = loader->head;
	while(handler!=NULL)
	{
		if(handler->handler_initial==NULL)
		{
			handler=handler->next;
			continue;
		}
		handler->handler_initial(handler,handler_ini);			//可能有一些handler特有的配置项，此initial用于读取这些配置项（保存在handler->info指针中）

		for(i=0;i<handler->n_worker;i++)
		{
			//创建相应的worker
			pid = fork();
			if(pid==0)	//child
			{
				if(handler->handler_work!=NULL)
				{
					while(1)
					{
						handler->handler_work(NULL);
					}
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
	log_notice("all handler start ...\n");
	return 0;	

}

/*
 * 函数作用：根据参数key指定的键，来调用read_task_from_redis（此函数会根据key值读取redis，并将读取到的相应task代码部分保存到一个文件中。返回的task中保存着代码文件的路径)
 * 随后，根据task结构中的task_type字段，将此任务分配到某个handler的worker或者公共worker（具体来说，是发送到对应的消息队列中）
 */
int assign_task(const char *key)
{
	//从redis中读取数据...，并开始分配
	task_t task;
	memset(&task,0,sizeof(task));
	log_error("---------------------------------------------------------------");
	log_notice("从列表中获取到key:%s\n",key);
	int iRet = read_task_from_redis(key,&task);	
	if(iRet ==-1)
	{
		return -1;
	}
	
	//准备测试数据：保存在/data/oj/match/question/test.in\test.out
	log_notice("准备测试数据...");
	char test_data_dir [1024];
	snprintf(test_data_dir,1024,"%s/%ld/%ld",SOURCE_DIR,task.match_num,task.ques_num);
	char old_dir[1024];
	getcwd(old_dir,sizeof(old_dir));
	chdir(test_data_dir);
	log_notice("cd %s",test_data_dir);

	iRet = access("test.in",0);
	if(iRet ==-1)
	{		
		//测试文件不存在
		char key [1024];
		snprintf(key,1024,"%ld.%ld.test.in",task.match_num,task.ques_num);
		log_notice("%s 不存在,将从redis中读取","test.in");
		if(save_file_from_redis(key,"test.in",S_IRUSR|S_IWUSR)==-1)
		{
			sleep(2);
			if(save_file_from_redis(key,"test.in",S_IRUSR|S_IWUSR)==-1)
			{
				sleep(2);
				if(save_file_from_redis(key,"test.in",S_IRUSR|S_IWUSR)==-1)
				{
					
					return -1;
				}
			}
		}
	}
	iRet = access("test.out",0);
	if(iRet ==-1)
	{
		//测试文件不存在
		char key [1024];
		snprintf(key,1024,"%ld.%ld.test.out",task.match_num,task.ques_num);
		log_notice("%s 不存在,将从redis中读取","test.out");
		if(save_file_from_redis(key,"test.out",S_IRUSR|S_IWUSR)==-1)
		{
			sleep(2);
			if(save_file_from_redis(key,"test.out",S_IRUSR|S_IWUSR)==-1)
			{
				sleep(2);
				if(save_file_from_redis(key,"test.out",S_IRUSR|S_IWUSR)==-1)
				{
					
					return -1;
				}
			}
		}
	}
	
	chdir(old_dir);
	
	snprintf(test_data_dir,1024,"%s/%d/%d",SOURCE_DIR,task.match_num,task.ques_num);
	
	
	//现在是分配任务
	handler_interface_t *handler = loader->head;
	
	//1. handler有空闲--->handler  						
	//2. handler无空闲，且公共的也没有空闲
	while(handler)
	{			
		if(strncasecmp(task.task_type,handler->name,strlen(handler->name))==0)
		{
			log_info("匹配handler:%s\n",handler->name);
			//添加后缀
			char old_dir [1024];
			//保存当前目录
			getcwd(old_dir,sizeof(old_dir));
			//进入到task的目录中
			chdir(task.dir);		
			if(handler->suffix[0]=='\0')
			{
				//未设置suffix后缀，则用handler名即task_type来作后缀，适用于c、java、php等
				snprintf(task.source,sizeof(task.source),"%s.%s",task.name,task.task_type);				
			}
			else
			{
				//设置了suffix后缀名，则用相应的后缀名来处理，适用于c++、python
				if(handler->suffix[0]=='.')//后缀名中已经有.
				{
					snprintf(task.source,sizeof(task.source),"%s%s",task.name,handler->suffix);	
				}
				else//后缀名中无.，则自己添加
				{
					snprintf(task.source,sizeof(task.source),"%s.%s",task.name,handler->suffix);	
				}
			}
			rename(task.name,task.source);
			log_info("添加后缀名为文件为：%s\n",task.source);
			
			//构造消息内容，待发送给具体某个worker
			mymsg_t msg;
			msg.mtype = 1;
			int msg_len = sizeof(task_t);
			memcpy(&msg.task,&task,msg_len);		
			//找到相应适配的handler
			
			if(sem_get(handler->sem_id)>0 || sem_get(loader->sem_id)<=0 )
			{
				//相应的handler可以执行任务，或者公共worker也没有空闲来处理，则直接发送到专属的handler的消息队列来等待处理
				
				/*
				 * 注意，judger端是无论列表中有多少个新任务对应的key，都一并读取下来。前提是，core端可以完成好任务的分配工作
				 * 否则，会导致相应handler的消息队列中一直堆积着新任务
				 */
				
				//将任务发送给它
				int iRet = msgsnd(handler->msg_id,&msg,sizeof(msg.task),0);	
				if(iRet == -1)
				{
					log_error("msgsnd error. errno :%d",errno);
					log_notice("%d,%d",handler->msg_id,msg_len);
					//log_notice("match_num:%d\n\
								ques_num:%d\n\
								user_num:%d\n\
								commit_num:%d\n\
								dir:%s\n\
								source:%s\n\
								name:%s\n\
								task_type:%s\n\
								text:%s\n\
								",msg.task.match_num,msg.task.ques_num,msg.task.user_num,msg.task.commit_num,msg.task.dir,msg.task.source,msg.task.name,msg.task.task_type,msg.task.text);
					perror("msgsnd");
					return -1;
				}
				log_info("将任务发送给了handler[%s][%d]...%d\n",handler->name,handler->msg_id,iRet);
			}
			else  if(sem_get(loader->sem_id)>0)
			{
				//专属的handler并没有空闲的进程可以来执行任务，这时候看一看公共的worker
				//公共worker可以执行任务
				log_info("common worker。。\n");
				msgsnd(loader->msg_id,&msg,msg_len,0);
			}
			else
			{
				//程序不会执行到时这里
				log_info("程序不会执行到这里的..\n");
			}
			break;
		}				
		handler = handler->next;
	}
	return 0;
}

/*
 * 函数作用：judger的主循环，它会不断地监听相应的redis list，并从中得到key值，用来调用assign_task
 */
int work()
{
	log_notice("初始化工作已经完成，[%s]开始工作....",conn_conf->host_name);
	//todo:
	//向core发送消息(认证？以及配置文件中所设置的worker数量：包括公共的worker，也包括handler特有的handler)
	//随后，core发送过来key，要不就直接通过redis的缓存来发吧
	//
	
	//list名，core向list写消息，内容是一个相应的key
	const char *judger_list_name = conn_conf->redis_list;
	printf("conn_conf_redis_list:%s\n",conn_conf->redis_list);
	int list_len;
	log_notice("start to monitor list.....\n");
	
	while(1)
	{	/*
		 * todo：这里初始化key，可以是读取redis中的队列得到key[question:为什么不直接把任务hash放进列表里，直接取一个任务就好呢？因为list是存放字符串的列表，不可以存放hash]
		 */
		redisReply *reply ;
		//一直等待，直到列表中有新的key。		
		reply = redisCommand(redis_conn,"brpop %s 0",judger_list_name); 
		
		//注意，这里从list中提取一个key，但它返回的reply是一个array 类型：元素1是列表名，元素2是对应索引上的值
		if(reply->type==REDIS_REPLY_ARRAY)
		{
			if(reply->elements==2)
			{
				redisReply *elem=reply->element[1];
				if(elem->type==REDIS_REPLY_STRING)
				{
					assign_task(elem->str);
				}				
			}
		}
		else if(reply->type==REDIS_REPLY_ERROR)
		{
			log_error("error : %s",reply->str);
		}
		else
		{
			
		}		
		freeReplyObject(reply);		
	}
}
void sig_int(int sig)
{
	if(sig==SIGINT)
	{
		printf("finish...\n");
		finish();
	}
}
void  main()
{
	log_initial("judger1",LOG_PID,LOG_USER,0);
	log_info("[%s]start to initial...\n",JUDGER_NAME);
	log_info("start to initial connect [redis & core & mysql]....\n");
	log_error_s("rB","--------------------------------------------------------------------------------------");
	if(init_connect()==-1)
	{
		log_error("error to initial connect ,exit");
		exit(1);
	}
	
	log_error_s("rB","--------------------------------------------------------------------------------------");
	handler_loader();
	start_all_handler();
	sleep(1);
	//初始化完成，开始工作
	signal(SIGINT,&sig_int);
	work();
}

