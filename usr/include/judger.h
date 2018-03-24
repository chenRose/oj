#ifndef _JUDGER_H_
#define _JUDGER_H_

#include "log.h"
#include "iniparser.h"

typedef struct {
	const char *core_ip;
	int  core_port;
	const char *redis_ip;
	int  redis_port;
	const char *redis_passwd;
}judger_conf_t;
struct HANDLER_INTERFACE_T ;
typedef int (*PF_work)(void );
typedef int (*PF_reload)(struct HANDLER_INTERFACE_T *,dictionary*);
typedef int (*PF_initial)(struct HANDLER_INTERFACE_T* ,dictionary*);
typedef int (*PF_finish)();



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


	pid_t pids[64];
	
	PF_initial initial;			//read conf && check the compiler exist
	PF_work work;				//worker 
	PF_reload reload;			//reload conf
	PF_finish finish;				//release resource
	int msg_id;					//IPC mq_id

}handler_interface_t;


//file judger_handler.ini 
//list为模块（handler）组织而成的链表
//其它信息为global 节的信息
typedef struct {				
	const char *dir;				//
	int num;
	handler_interface_t *head;
	handler_interface_t *tail;
}loader_interface_t;

typedef struct {
	long mtype;

	char cmd[2048];

}mymsg_t;

int default_load_ini(loader_interface_t *loader,handler_interface_t *handler,dictionary *handler_ini,int seqnum,int msg_id)
{
		handler->msg_id=msg_id;
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
}
#endif
