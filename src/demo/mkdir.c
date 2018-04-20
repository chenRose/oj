#include<fcntl.h>
#include <sys/types.h>
 #include <sys/stat.h> 
#include<stdio.h>
int main()
{
	int status; 

	status = mkdir("/home/newdir/a/b/c", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		
	char buf[1024];
	
	snprintf(buf,1024,"hehllo %s ","123");
	printf(buf);
	printf("\n");
	snprintf(buf,1024,"%s%s",buf,"world");
	printf(buf);
	

	printf("%d" ,status); 
	return 0;
}
