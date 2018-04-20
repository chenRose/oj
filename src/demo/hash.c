#include "hiredis/hiredis.h"
#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
char buf[1024*1024];
int readFile(const char * file)
{
	int fd = open(file,O_RDONLY);
	if(fd<=0)
	{
		printf("open failure");
	}
	int nbytes = read(fd,buf,1024*1024);
	printf("read %d bytes \n",nbytes);
	return 0;
}
int writeFile(const char * file,const char *buf,int len)
{
	int fd = open(file,O_WRONLY|O_CREAT);
	printf("fd is %d \n",fd);
	write(fd,buf,len);	
	return 0;

}
void testString(redisContext *redis )
{
	readFile("/home/oj/src/core.c");
    redisCommand(redis,"set %s %s","1234",buf);
    redisReply *reply=redisCommand(redis,"get %s","1234");
    if(reply->type != REDIS_REPLY_STRING)
    {
        printf("command execute failure:%s\n");
        return ;
    }

    printf("GET test:%s\n", reply->str);

}
void testHash(redisContext *redis)
{
    readFile("/home/oj/src/core.c");
    redisReply *reply;
    redisCommand(redis,"HMSET %s text %s other %s","2014150216",buf,"chenshuyu");
	    reply = redisCommand(redis,"HGETALL 2014150216");
    if(reply->type == REDIS_REPLY_ARRAY)
    {
        printf("receive successful: %d \n",reply->len);
        printf("integer : %d \n len: %d \nstr:%s \n,elements:%d\n",reply->integer,reply->len,reply->str,reply->elements);
    }

    int i ;
    for(i=0;i<reply->elements;i++)
    {
        redisReply * elem = reply->element[i];
        if(elem->type == REDIS_REPLY_STRING)
        {
            if(strncmp(elem->str,"text",elem->len)==0)
            {
                i++;
                elem= reply->element[i];
                writeFile("/home/oj/src/core.c.bak",elem->str,elem->len);
                printf("write done ...");
            }
        }
    }

}

int main(int argc,char *argv[])
{
	if(argc!=4)
	{	
		printf("error ");
		printf("1:key,2:file,3:type\n");
		exit(1);
	}
	printf("key:\t%s\n",argv[1]);
	printf("text:\t%s\n",argv[2]);
	printf("type:\t%s\n",argv[3]);
	redisContext *redis = redisConnect("127.0.0.1",6379);

	if(redis!=NULL && redis->err)
	{
		printf("error connect");
		return -1;
	}
	else
	{
		printf("connect successful \n");
	}
	redisCommand(redis,"AUTH myoj");

	redisReply* reply;
	//hash
	readFile(argv[2]);

    reply = redisCommand(redis,"HMSET %s text %s match_num %ld user_num %ld ques_num %ld commit_num %ld task_type %s",argv[1],buf,201804020001,2014150216,2018040200010001,1,argv[3]);
	if(reply->type==REDIS_REPLY_ERROR)
	{
		printf("error,%s\n",reply->str);
	}	

	
}
