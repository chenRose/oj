#include<stdio.h>
#include<stdlib.h>
#include<stdarg.h>
#include<syslog.h>
char buffer[1024];


int myfun(const char *f,char *format ,...)
{
	if(f==NULL)
	{
		printf("empty");
	}
	va_list ap;
	va_start(ap,format);
	int n=vsprintf(buffer,format,ap);
	
	//syslog(LOG_ERR,format,ap);	
	va_end(ap);
	printf(buffer);
	syslog(LOG_ERR,buffer);
	return n;
}

void main()
{
	myfun(NULL,"helo %d,%s",123,"world");
}
