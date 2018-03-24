#include<stdio.h>
#include<sys/select.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include "log.h"
#include<string.h>
#include "iniparser.h"
#define LISTENQ 5



enum report_type{task,report};

//
typedef struct HANDLER_LOAD_INFO_T
{
	char name[10];
	short total;			
	short pending;
}handler_load_info_t;
typedef struct CLIENT_MSG_T
{
	int report_type;					//new task (from php) or report of judger
	int info_num;						
	handler_load_info_t load[10];		//结构中只有10个handler_load_info_t结构的容量，而info_num为实际个数，如果info_num超过10，说明还可以再读取info_num-10个handler_load_info结构
	void *meg;
}client_msg_t;

typedef struct CLIENT_INFO_T
{
	int sockfd;					//set by server(after accept)
	struct sockaddr *sockaddr;	//client addr
	int clilent;
	
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
			}
			client=client->next;
		}	
	}	
	return 0;
}
