#include<stdio.h>
#include<sys/ptrace.h>
#include<sys/resource.h>
#include<sys/reg.h>
#include "common.h"
#include<libgen.h>
#include<sys/user.h>
#include<sys/signal.h>
#include "okcalls.h"
handler_interface_t *g_handler;
int call_counter[512]={0};
#define MB 1024*1024;
typedef struct OJ_RUN_LIMIT
{
	int time_limit_soft;
	int time_limit_hard;
	int file_limit_soft;
	int file_limit_hard;
	int pro_limit_soft;
	int pro_limit_hard;
	int mem_limit_soft;
	int mem_limit_hard;
	
}oj_run_limit;

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
int compile(task_t *task,OJ_STATUS *oj_status);
int ptrace_program(int pid,OJ_STATUS *oj_status);
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
	init_syscalls_limits();
	return 0;
}

int compile(task_t *task,OJ_STATUS *oj_status)
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
		limit.rlim_cur = 3;
		limit.rlim_max = 6;
		setrlimit(RLIMIT_CPU,&limit);
		alarm(12);
		//设置创建文件限制
		limit.rlim_cur = 100 * MB;
		limit.rlim_max = 100 * MB;
		setrlimit(RLIMIT_FSIZE,&limit);
		//设置存储限制
		limit.rlim_cur = 1024*MB;//1GB
		limit.rlim_max = 1024*MB;//1GB
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
		int status = ptrace_program(pid,oj_status);
		if(*oj_status==OJ_AC)
		{
			if(status==0)
			{
				log_notice_s("编译成功:,可执行文件保存为%s/%s",task->dir,task->name);
				return 0;
			}
			else
			{
				log_error("compile:[%s] error [result:%d]",task->source,status);
				log_error("编译错误信息为:");
				char CE_error[1024];		
				snprintf(CE_error,1024,"cat %s.ce_error",task->name);
				system(CE_error);
				return -1;
			}
		}
		else
		{
			log_error_s("rB","编译失败：%s",*oj_status==OJ_OLE?"创建文件过大":
											*oj_status==OJ_TLE?"时间超限":
											*oj_status==OJ_MLE?"内存超限":"其它错误");
			return -1;
		}
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

int ptrace_program(int pid,OJ_STATUS *oj_status)
{
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
			log_error("更新...%d->%d",topmemory,tempmemory);
			topmemory = tempmemory;				
		}
		if(temprss>toprss)
		{
			log_error("更新...%d->%d",toprss,temprss);
			toprss = temprss;					
		}
		if(WIFEXITED(status))
		{
			log_info("正常终止。退出状态是：%d\n",WEXITSTATUS(status));
			return WEXITSTATUS(status);
		}
		else if(WIFSIGNALED(status))
		{
			//例如，时间超限、内存超限等问题
			int signal = WTERMSIG(status);
			log_info("异常终止子进程。信号编号为：%d[%s]\n",signal, strsignal(signal));
			if(*oj_status == OJ_AC)
			{
				switch(signal)
				{
					//时间超限
					case SIGKILL:
						log_error("who killed me?");
					case SIGXCPU:
					case SIGALRM:
						*oj_status = OJ_TLE;
						break;
						//打开文件超限
					case SIGXFSZ:
						*oj_status = OJ_OLE;
						break;
					default :
						*oj_status = OJ_RE;
				}
			}
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
					*oj_status=OJ_RE;
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
	if(*oj_status!=OJ_AC)
	{
		log_error("进程 return %d",-1);
		return -1;
	}
	else if (*oj_status==OJ_AC)
	{
		log_error_s("rBU","有问题...");
	}
}
int get_run_limit(oj_run_limit *r_limit)
{
	r_limit->time_limit_soft=3;
	r_limit->time_limit_hard=2*r_limit->time_limit_soft;
	r_limit->file_limit_soft = 200 *MB;
	r_limit->file_limit_hard = 1.2 * r_limit->file_limit_soft;
	r_limit->pro_limit_soft = 1;
	r_limit->pro_limit_hard= 1.2 * r_limit->pro_limit_soft;
	r_limit->mem_limit_soft = 300*MB;
	r_limit->mem_limit_hard = 2*r_limit->mem_limit_soft;
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
		log_info("[%s:%d]get a task，file will store in%s/%s\n",g_handler->name,getpid(),task->dir,task->source);
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

	int status = compile(task,&oj_status);	
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
		log_notice("即将运行程序：%s/%s",task->dir,task->name);
			
		//编译成功了，需要继续对编译完成的程序进行执行		
		//3. 准备一些测试数据
		oj_run_limit r_limit;
		//get_test_data(&r_limit);	
		get_run_limit(&r_limit);

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
			/*if(freopen(IN, "r", stdin)==NULL)
			{
				perror("freopen");
				log_error("运行程");
			}*/
			
			if(freopen(OUT, "a+", stdout)==NULL)
			{
				perror("freopen");
				log_error("运行程2");
			}
			if(freopen(ERROR, "a+", stderr)==NULL)
			{
				perror("freopen");
				log_error("运行程3");
			}
		
			
			/*
			 * todo :降低权限 
			 */
			 
			 /*
			  * todo :设置相应的限制
			  */
		
			struct rlimit limit;	
			//时间限制
			limit.rlim_max= r_limit.time_limit_hard;
			limit.rlim_cur=r_limit.time_limit_soft;
			setrlimit(RLIMIT_CPU,&limit);
			alarm(0);
			alarm(limit.rlim_max*10);			
			//文件限制
			limit.rlim_max = r_limit.file_limit_hard;
			limit.rlim_cur = r_limit.file_limit_soft;
			setrlimit(RLIMIT_FSIZE,&limit);
			
			
			//内存限制
			limit.rlim_max = r_limit.mem_limit_hard;
			limit.rlim_cur = r_limit.mem_limit_soft;
			setrlimit(RLIMIT_AS,&limit);
			//进程限制
			limit.rlim_max = r_limit.pro_limit_hard;
			limit.rlim_cur = r_limit.pro_limit_soft;
			setrlimit(RLIMIT_NPROC,&limit);
			//开始执行程序
			//先构造执行程序的命令
			
			char cmd[30];
			snprintf(cmd,sizeof(cmd),"./%s",task->name);

			//const char *execute_cmd [] = {cmd,cmd,NULL};
			log_notice("开始运行程序:%s %d.%d\n",cmd,getpid(),getppid());
			ptrace(PTRACE_TRACEME, 0, NULL, NULL);
			
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
			ptrace_program(pid,&oj_status);		
			if(oj_status==OJ_AC)
			{
				//结果比较
				int iRet = compare_files("../a","../b");
				printf("ret:%d\n",iRet);
			}
			else
			{				
				log_notice(str_oj_status[oj_status]);
			}
			
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
	free(task);
	return 0;
}

int handler_get_available()
{
	if(g_handler==NULL)
	{
		log_error_s("rB","handler未初始化");
		return 0;
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