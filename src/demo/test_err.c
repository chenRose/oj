#include<stdio.h>
#include <stdlib.h>  
#include <sys/wait.h>  
#include <sys/types.h>  
int main()
{
	chroot("/home/oj/src/demo");
	
	int status = execvp("gcc","gcc","e_include.c",NULL);
	printf("status:%d\n",status);
	if(status==-1)
	{
		perror("execvp");
	}
	printf("exit status value = [0x%x]\n", status);  
    if (WIFEXITED(status))  
     {  
            if (0 == WEXITSTATUS(status))  
            {  
                printf("run shell script successfully.\n");  
            }  
            else  
            {  
                printf("run shell script fail, script exit code: %d\n", WEXITSTATUS(status));  
            }  
        }  
        else  
        {  
            printf("exit status = [%d]\n", WEXITSTATUS(status));  
        }  	
}
