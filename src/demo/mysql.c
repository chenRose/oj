#include<stdio.h>
#include<mysql/mysql.h>
#include<stdlib.h>
void main()
{
	//先初始化mysql库
	if (mysql_library_init(0, NULL, NULL))  
    {  
        printf("could not initialize MySQL library\n");  
        getchar();  
        exit(1);  
    }  	
	//连接上下文：类型为MYSQL
	MYSQL conn;
	//初始化
	mysql_init(&conn);

	MYSQL *ret = mysql_real_connect(&conn,
									"127.0.0.1", //ip
									"root",//用户
									"myoj",//密码
									"oj",//数据库
									0,NULL,0);
	if(ret == NULL)
	{
		printf("connect error!\n");
	}
	
	mysql_close(&conn);
	mysql_library_end();
}
