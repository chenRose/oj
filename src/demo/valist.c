#include<stdio.h>
#include<stdlib.h>
#include<stdarg.h>
#include<syslog.h>
char buffer[1024];


int myfun(char *format ,...)
{
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
	myfun("helo %d,%s",123,"world");
}
