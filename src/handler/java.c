#include<stdio.h>
#include<sys/ptrace.h>
#include<sys/resource.h>
#include<sys/reg.h>
#include "common.h"
#include<libgen.h>
#include<sys/user.h>
#include "okcalls.h"
handler_interface_t *g_handler;
int call_counter[512]={0};


/*
 *   handler默认的接口：
 *    
 */
int handler_initial(handler_interface_t *handler,dictionary *handler_ini);
int handler_finish();
int handler_work(void * r_task);
int handler_get_capacity(void);
int handler_get_available(void);
int handler_reload(handler_interface_t *handler,dictionary* handler_ini);
/*
 * 其它被调用的函数
 */
void init_syscalls_limits();
int compile(task_t *task);
int get_proc_status(int pid, const char * mark);

void init_syscalls_limits()
{
	int i ;
	for (i = 0; i==0||LANG_CV[i]!=-1; i++)
	{
		call_counter[LANG_CV[i]] = HOJ_MAX_LIMIT;	
	}
	
}

int handler_initial(handler_interface_t *handler,dictionary *handler_ini)
{
	if(handler==NULL)
	{
		log_error("error ,argument \"handler\" is NULL");
	}
	g_handler=handler;
	return 0;
}

int compile(task_t *task)
{
	//先对文件名进行处理：
	log_info("compile task:%s/%s\n",task->dir,task->source);
	const char *compile_cmd [] = {"gcc",task->source,"-o",task->name,"-fno-asm", "-Wall",
			"-lm", "--static", "-std=c99", "-DONLINE_JUDGE",NULL};
	log_notice("开始编译文件：[%s]\n",task->source);
	
	pid_t pid=fork();
	if(pid==0)
	{
		//子进程
		//先设置一些限制
		struct rlimit limit;
		//设置CPU限制
		limit.rlim_cur = 60;
		limit.rlim_max = 60;
		setrlimit(RLIMIT_CPU,&limit);
		alarm(60);
		//设置创建文件限制
		limit.rlim_cur = 100 * 1024*1024;
		limit.rlim_max = 100 * 1024*1024;
		setrlimit(RLIMIT_FSIZE,&limit);
		//设置存储限制
		limit.rlim_cur = 1024*1024*1024;//1GB
		limit.rlim_max = 1024*1024*1024;//1GB
		setrlimit(RLIMIT_AS,&limit);
		
		//保存相应的错误信息的路径
		char CE_error[1024];		
		snprintf(CE_error,1024,"%s.ce_error",task->name);
		freopen(CE_error, "w", stderr);	
		
		/*
		todo：执行编译的权限设置
		*/
		execvp(compile_cmd[0],(char * const *)compile_cmd);	
	}
	else
	{
		//父进程
		int status;
		//等待子进程
		waitpid(pid,&status,0);
		if(status==0)
		{
			log_notice("compile:[%s] successful![result:%d]",task->source,status);
			if(WIFEXITED(status))
			{
				log_notice("正常终止。退出状态是：%d\n",WEXITSTATUS(status));
			}
			else if(WIFSIGNALED(status))
			{
				log_error("异常终止子进程。信号号码为：%d%s\n",WTERMSIG(status));
			
			}
			else if(WIFSTOPPED(status))
			{
				log_notice("子进程暂停，编号 为:%d\n",WSTOPSIG(status));
			}
			
		}	
		else
		{
			log_error("compile:[%s] error ![result:%d]",task->source,status);
			log_error("编译错误信息为:");
			char CE_error[1024];		
			snprintf(CE_error,1024,"cat %s.ce_error",task->name);
			system(CE_error);
		}
		return status;		
	}
	
}
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


int handler_work(void * r_task)
{
	
	task_t *task;
	char file_path[1024];
	if(r_task==NULL)//工作模式：handler特有的worker
	{
		log_notice("[%s:%d]\tworker wating a task...[%d]",g_handler->name,getpid(),g_handler->msg_id);
		mymsg_t *msg=malloc(sizeof(mymsg_t));	
		int iRet = msgrcv(g_handler->msg_id,msg,2048,0,0);//block until new command

		task = malloc(sizeof(task_t));
		memcpy(task,&(msg->task),sizeof(task_t));
		log_info("[%s:%d]get a task，file path is%s/%s\n",g_handler->name,getpid(),task->dir,task->source);
		free(msg);
		sem_p(g_handler->sem_id);
	}
	else
	{
		task = (task_t*)r_task;	
	}
	
	//获取了当前工作的task_t结构信息
	
	//保留原来的工作目录
	char old_dir[1024];
	getcwd(old_dir,sizeof(old_dir));
	

	OJ_STATUS oj_status = OJ_AC;
	//0. 先进入相应的目录
	if(chdir(task->dir)==-1)
	{
		log_error("error path:%s\n",task->dir);
		return -1;
	}
	log_notice("进入新的目录：%s\n",task->dir);
	//1. 先进行编译工作
	log_error("---------------------------------------------------------------");
	log_notice("[信号量值为：%d]\n",sem_get(g_handler->sem_id));

	int status = compile(task);	
	//2. 之后，将要进行运行的操作
	if(status != 0)
	{
		//编译失败，可以直接返回，而不用接着执行
		oj_status=OJ_CE;
		//todo:把结果写到数据库中		
	}
	else
	{
		log_notice("编译成功：%s/%s",task->dir,task->name);
		init_syscalls_limits();
	
	
		//编译成功了，需要继续对编译完成的程序进行执行		
		//3. 准备一些测试数据
		//get_test_data();
		//4. 运行程序
		pid_t pid;
		pid=fork();
		if(pid==0)
		{
			//child		
			//程序的输入输出和错误，都使用文件的形式
			char IN [30];
			char OUT [30];
			char ERROR[30];
			snprintf(IN,30,"%s.in",task->name);
			snprintf(OUT,30,"%s.out",task->name);
			snprintf(ERROR,30,"%s.error",task->name);
			freopen(IN, "r", stdin);
			freopen(OUT, "a+", stdout);
			freopen(ERROR, "a+", stderr);
			ptrace(PTRACE_TRACEME, 0, NULL, NULL);
			/*
			 * todo :降低权限 
			 */
			 
			 /*
			  * todo :设置相应的限制
			  */
			//时间限制
			int time_limit = 3;
			struct rlimit limit;	
			limit.rlim_max=time_limit+3;
			limit.rlim_cur=time_limit;
			setrlimit(RLIMIT_CPU,&limit);
			alarm(0);
			alarm(limit.rlim_max*10);			
			//文件限制
			int file_limit = 200*1024*1024;//200M
			limit.rlim_max = file_limit*1.2;
			limit.rlim_cur = file_limit;
			setrlimit(RLIMIT_FSIZE,&limit);
			//内存限制
			int mem_limit = 300*1024*1024;	//300M
			limit.rlim_max = mem_limit;
			limit.rlim_cur = mem_limit*1.2;
			setrlimit(RLIMIT_AS,&limit);
			//进程限制
			int pro_limit = 1;
			limit.rlim_max = pro_limit*1.2;
			limit.rlim_cur = pro_limit;
			setrlimit(RLIMIT_NPROC,&limit);
			//开始执行程序
			//先构造执行程序的命令
			char cmd[30];
			snprintf(cmd,sizeof(cmd),"./%s",task->name);
			//const char *execute_cmd [] = {cmd,cmd,NULL};
			log_notice("开始运行程序:%s %d.%d\n",cmd,getpid(),getppid());
			int iRet = execl(cmd,cmd,(char*)NULL);	
			if(iRet ==-1)
			{
				perror("execl");
			}
		}
		else if(pid==-1) 
		{
			log_error("fork error.%d",__LINE__);
		}
		else
		{
			//父进程：新监控子进程的执行
			log_notice("父进程开始监视子进程%d...",pid);
			int status;
			struct rusage resource;
			struct user_regs_struct reg; 
			int i =0;
			
			int topmemory=0,toprss=0;
			int tempmemory=0,temprss=0;
			int flags = 0;
			while(1)
			{
				//使用wait4,除了常规的wait功能外，还可以获取子进程的一些资源使用情况
				/*
				 * 如，CPU时间总量，系统CPU时间总量等。
				 */
				wait4(pid,&status,0,&resource);
				tempmemory = get_proc_status(pid,"VmPeak:");
				temprss = get_proc_status(pid,"VmRSS:");
				if (tempmemory > topmemory)
				{
					topmemory = tempmemory;
				//	log_error("更新...");
				}
				if(temprss>toprss)
				{
				//	log_error("更新...%d->%d",toprss,temprss);
					toprss = temprss;
					
				}
				if(WIFEXITED(status))
				{
					log_info("正常终止。退出状态是：%d\n",WEXITSTATUS(status));
					break;
				}
				else if(WIFSIGNALED(status))
				{
					//例如，时间超限、内存超限等问题
					log_info("异常终止子进程。信号号码为：%d[%s]\n",WTERMSIG(status), strsignal(WTERMSIG(status)));
					break;
				}
				else if(WIFSTOPPED(status))
				{
					
					//例如，系统调用..
					int orig_rax,rax;
					if(WSTOPSIG(status)==5)
					{
						orig_rax = ptrace(PTRACE_PEEKUSER,pid,8*ORIG_RAX,NULL);
						//log_info("调用编号[%d]\n",orig_rax);						
						if(call_counter[orig_rax]==0)
						{
							oj_status=OJ_RE;
							log_notice("call_counter[%d]==0\n",orig_rax);
							log_error("fobbiden system call:%d",orig_rax);
							log_error("kill it ...");
							ptrace(PTRACE_KILL, pid, NULL, NULL);
						}
						else
						{
							call_counter[orig_rax]--;
						}
					}
				}		
				ptrace(PTRACE_SYSCALL,pid,NULL,NULL);
			}
			log_notice("max rss:%dKB(rusage)",resource.ru_maxrss);
			log_notice("max rss:%dKB",toprss);
			log_notice("topmemory:%dKB(%d)(vmpeak)",topmemory,pid);
		}
	}
	if(r_task==NULL)
	{
		sem_v(g_handler->sem_id);
	}
	if(oj_status==OJ_AC)
	{
		log_notice("执行成功..\n");		
	}
	else if (oj_status==OJ_CE)
	{
		log_error("编译失败...");
	}
	else 
	{
		log_error("运行时错误...");
	}
	chdir(old_dir);
	log_notice("[信号量值为：%d]\n",sem_get(g_handler->sem_id));
	log_error("---------------------------------------------------------------");
	
	return 0;
}

int handler_get_available()
{
	if(g_handler==NULL)
	{
		log_error_s("rB","handler未初始化");
		return 1<<31;
	}
	int free = sem_get(g_handler->sem_id);
	if(free==0)
	{
		//返回堆积量的负数
		int ret = msg_get_qnum(g_handler->msg_id);
		if(ret==-1)
		{
			//除错：返回一个超过n_worker数的值
			return g_handler->n_worker+1;	
		}
		else
			return -ret;
	}
	else
	{
		return free;
	}
}
int handler_get_capacity()
{	
	return g_handler->n_worker;
}
int handler_reload(handler_interface_t *handler,dictionary* handler_ini)
{
	printf("handler_reload...\n");
	return 0;
}
int handler_finish()
{
	printf("handler_finish...\n");
	return 0;
}
