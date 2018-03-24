#ifndef OJ_LOG_H
#define OJ_LOG_H

#include<stdio.h>
#include<stdlib.h>
#include<syslog.h>
#include<stdarg.h>



char error_buf[1024];//todo:  buffer size 
char info_buf[1024];
char buffer[1024];

int log_initial(const char *ident,int option,int facility,int console_flag);
int log_writelog(int priority,const char *format);
int log_finish();


int console_flag=1;//control if output to console


int log_initial(const char *ident,int option,int facility,int console)
{
	
	openlog(ident,option,facility);
	sprintf(error_buf,"\033[41;30m");
	sprintf(info_buf,"\033[41;32m");
	if(console!=0)
	{
		console_flag=0;
	}
	
	return 0;
}

int log_error(char *format,...)
{
	va_list ap;
	va_start(ap,format);
	int n =vsprintf(buffer,format,ap);
	sprintf(error_buf,"\033[41;30m%s\033[0m\n",buffer);

	va_end(ap);
	syslog(LOG_ERR,buffer);
	if(console_flag)
	{
		printf(error_buf);
	}
	return n;
}

int log_info(char *format,...)
{
    va_list ap;
    va_start(ap,format);
    int n =vsprintf(buffer,format,ap);
	sprintf(info_buf,"\033[40;32m%s\033[0m\n",buffer);
    va_end(ap);
    syslog(LOG_INFO,buffer);
    if(console_flag)
    {
        printf(info_buf);
    }
    return n;
	
}



int log_finish()
{
	closelog();
}
#endif
