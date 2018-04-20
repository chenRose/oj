#include<stdio.h>
#include<sys/select.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include "log.h"
#include "common.h"
#include<string.h>
#include "iniparser.h"
#define LISTENQ 5



enum report_type{task,report};

//

typedef struct CLIENT_INFO_T
{
	int sockfd;					//set by server(after accept)
	struct sockaddr_in sockaddr;	//client addr
	int client_type;
	int enable ;
	handler_load_info_t *load;
	int n_use;
	int n_handler;
	struct CLIENT_INFO_T *next;
	struct CLIENT_INFO_T *prev;
}client_info_t;
typedef struct CLIENT_LIST_T
{
	int n_client;
	client_info_t *head;
	client_info_t *tail;
}client_list_t;
client_list_t *clients;

int upload_client_load(client_info_t *client,client_msg_t *msg,int is_register)
{
	//注册judger,需要分配client中的load对象
	if(is_register == 1)
	{
		if(msg->info_num<=MSG_HANDLER_NUM)
		{
			client->load = malloc(sizeof(handler_load_info_t)*MSG_HANDLER_NUM);
			client->n_handler=MSG_HANDLER_NUM;
		}
		else
		{
			client->load = malloc(sizeof(handler_load_info_t)*msg->info_num);		
			client->n_handler=msg->info_num;
		}	
		//client->n_user = msg->info_num;
	}
	//judger发过的消息中update字段不为0，表示 handler数量或者位置有变化。若为0，则只更新负载信息
	if(msg->update==0)
	{
		int i ;
		if(msg->info_num<=MSG_HANDLER_NUM)
		{
			for(i=0;i<msg->info_num;i++)
			{
				client->load[i].total = msg->load[i].total;
				client->load[i].pending = msg->load[i].pending;				
			}
		}
		else			//消息中load有扩展，需要再读取一定字节的信息
		{
			read (client->sockfd,&client->load[MSG_HANDLER_NUM],sizeof(handler_load_info_t)*(msg->info_num-MSG_HANDLER_NUM));
		}				
	}
	else	
	{
		if(client->n_handler < msg->info_num )//需要重新分配内存load
		{		
			free(client->load);
			int i;
			client->load = malloc(sizeof(handler_load_info_t)*msg->info_num);	
			client->n_handler=msg->info_num;
	//		client->n_user = msg->info_num; 
			//拷贝load信息
			for(i=0;i<msg->info_num;i++)
			{
				strncpy(client->load[i].name,msg->load[i].name,sizeof(msg->load[i].name));
				client->load[i].total = msg->load[i].total;
				client->load[i].pending = msg->load[i].pending;				
			}			
		}
		else		
		{
			
		}
		
	}
}
int main()
{
	//初始化log接口
	log_initial("core",LOG_PID,LOG_USER,0);
		
	clients = malloc(sizeof(client_list_t));	
	memset(clients,0,sizeof(client_list_t));
	
	int listenfd,connfd;
	struct sockaddr_in cliaddr,servaddr;
	listenfd=socket(AF_INET,SOCK_STREAM,0);		//socket	
	bzero(&servaddr,sizeof(servaddr));			//清0服务端地址
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(44560);
	
	bind(listenfd,(struct sockaddr*) &servaddr,sizeof(servaddr));
	listen(listenfd,LISTENQ);
	
	int i;
	int maxfd = listenfd;
	int maxi=-1;
	int client[FD_SETSIZE];				//fd_set
	for(i=0;i<FD_SETSIZE;i++)
	{
		client[i]=-1;
	}
	fd_set rset,allset;
	FD_ZERO(&allset);
	FD_SET(listenfd,&allset);			//监控的socket
	int clilen;
	int nready;							//number of socket ready
	while(1)
	{
		rset=allset;
		nready = select(maxfd+1,&rset,NULL,NULL,NULL);
		
		if(FD_ISSET(listenfd,&rset))		//listenfd readable，new connect
		{
			clilen = sizeof(cliaddr);
			connfd = accept(listenfd,(struct sockaddr*)&cliaddr,&clilen);
			for(i=0;i<FD_SETSIZE;i++)
			{
				if(client[i]<0)
				{
					client[i]=connfd;
					break;					//在client数组中找一个位置
				}
			}
			if(i==FD_SETSIZE)
			{
				log_error("too many clients");
			}	
			FD_SET(connfd,&allset);		
			if(connfd>maxfd)
			{
				maxfd=connfd;			//update connfd
			}
			client_info_t *new_client = malloc(sizeof(client_info_t));
			new_client->sockfd = connfd;
			new_client->sockaddr=cliaddr;
			new_client->enable = 0;

			//新的judger连接：插入相应的clients中
			if(clients->n_client==0)
			{
				clients->head = new_client;
				clients->tail = new_client;
				clients->n_client++;
			}
			else
			{
				new_client->prev = clients->tail;
				clients->tail->next = new_client;
				clients->tail = new_client;
			}			
			if(--nready<=0)
			{
				continue;
			}
		}	
		client_info_t *client= clients->head;
		while(client)
		{
			if(FD_ISSET(client->sockfd,&rset))			{
				//某个sockfd可读，也就是：某个judger发送来信息了。此时需要把发送过来的信息读取
				client_msg_t msg ;
				
				int n = read(client->sockfd,&msg,sizeof(client_msg_t));
				if(n==sizeof(client_msg_t))
				{
					if(msg.report_type == 1)//register 
					{
						client->enable = 1;
						upload_client_load(client,&msg,1);
					}
					else
					{
						upload_client_load(client,&msg,0);
					}
				}
				else
				{
					
				}
				
			}
			client=client->next;
		}	
	}	
	return 0;
}
