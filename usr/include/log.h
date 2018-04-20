#ifndef OJ_LOG_H
#define OJ_LOG_H

#include<stdio.h>
#include<stdlib.h>
#include<syslog.h>
#include<stdarg.h>
#include<string.h>


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
	sprintf(error_buf,"\033[40;31m\33[1m%s\033[0m\n",buffer);

	va_end(ap);
	syslog(LOG_ERR,error_buf);
	if(console_flag)
	{
		printf(error_buf);
	}
	return n;
}
int _log_backend_s(const char *t ,char *format,va_list ap)
{
    int n =vsprintf(buffer,format,ap);
    if(t==NULL)
    {
        sprintf(info_buf,"\033[40;32m\33[1m%s\033[0m\n",buffer);
    }
    else
    {
        int i =0;
		int flags = 0;
        while(t[i]!='\0')
        {
            switch(t[i])
            {
                case 'b':
                    strcat(info_buf,"\033[40;30m");
                break;
                case 'r':
                    strcat(info_buf,"\033[40;31m");
                break;
                case 'g':
                    strcat(info_buf,"\033[40;32m");
                break;

                case 'y':
                    strcat(info_buf,"\033[40;33m");
                break;
                case 'w':
                    strcat(info_buf,"\033[40;37m");
                break;
                case 'l':
                    strcat(info_buf,"\033[40;34m");
                break;
                case 'p':
                    strcat(info_buf,"\033[40;35m");
                break;
                case 'B':
                    strcat(info_buf,"\033[1m");

                break;
                case 'U':
                    strcat(info_buf,"\033[4m");
                break;
				case 'n':
					flags = 1;
				break;
                case 'T':
                    strcat(info_buf,"\033[5m");
                break;
            }
            i++;
        }
        strcat(info_buf,buffer);
		if(flags==1)
	   	    strcat(info_buf,"\033[0m");
		else
			strcat(info_buf,"\033[0m\n");
    }
    va_end(ap);

}
int log_error_s(const char *t,char *format,...)
{
	memset(info_buf,0,sizeof(info_buf));
	va_list ap;
	va_start(ap,format);
	int n = _log_backend_s(t,format,ap);
	va_end(ap);
    syslog(LOG_ERR,info_buf);
    if(console_flag)
    {
        printf(info_buf);
    }
    return n;
	
}
int log_notice_s(const char *t,char *format,...)
{
	memset(info_buf,0,sizeof(info_buf));
    va_list ap;
    va_start(ap,format);
    int n = _log_backend_s(t,format,ap);

    va_end(ap);
    syslog(LOG_NOTICE,info_buf);
    if(console_flag)
    {
        printf(info_buf);
    }
    return n;

}
int log_notice(char *format,...)
{
    va_list ap;
    va_start(ap,format);
    int n =vsprintf(buffer,format,ap);
    sprintf(info_buf,"\033[40;32m\33[1m%s\033[0m\n",buffer);
    va_end(ap);
    syslog(LOG_NOTICE,info_buf);
    if(console_flag)
    {
        printf(info_buf);
    }
    return n;
}

int log_info(char *format,...)
{
    va_list ap;
    va_start(ap,format);
    int n =vsprintf(buffer,format,ap);
    va_end(ap);
    syslog(LOG_INFO,buffer);
    if(console_flag)
    {
        printf(buffer);
    }
    return n;	
}
int log_debug(char *format,...)
{
}
int log_finish()
{
	closelog();
}
#endif
