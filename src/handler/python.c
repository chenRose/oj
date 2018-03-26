#include<stdio.h>
#include "common.h"

handler_interface_t *g_handler;
int initial(handler_interface_t *handler,dictionary *handler_ini)
{
	if(handler==NULL)
	{
		printf("error");
	}
	g_handler=handler;
	return 0;
	
}
int work()
{
	
	printf("[%s:%d]\tworker start to work...\n",g_handler->name,getpid());
	while(1)
	{
		mymsg_t *msg=malloc(sizeof(mymsg_t));	
		int iRet = msgrcv(g_handler->msg_id,msg,2048,g_handler->seq_num,0);//block until new command 
        printf("[%s:%d]\tread %d bytes data:\n%s\n",g_handler->name,getpid(),iRet,msg->cmd);
		free(msg);
	}
	
	
	return 0;
}

int reload(handler_interface_t *handler,dictionary* handler_ini)
{
	printf("reload...\n");
	return 0;
}
