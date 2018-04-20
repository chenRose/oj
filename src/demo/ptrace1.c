#include <sys/ptrace.h>  
#include<sys/user.h>
#include <sys/types.h>  
#include <sys/wait.h>  
#include<stdio.h>
#include<string.h>
#include <unistd.h>  
#include <sys/reg.h>   /* For constants   
                                   ORIG_EAX etc */  
int main()  
{  
   pid_t child;  
    int orig_eax;  
    child = fork();  
    if(child == 0) {  
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);  
        execl("/bin/ls", "ls", NULL);  
    }  
    else {  
        wait(NULL);  
		struct user temp;
		memset(&temp,0,sizeof(temp));
		orig_eax = ptrace(PTRACE_PEEKUSER,   
                          child,
						 120,   
                          &temp);  
        int i ;
		char *str =( char*)(&temp);
		printf("%d\n",sizeof(temp));
		for(i=0;i<sizeof(temp);++i)
		{	
			printf("%c",str[i]);
		}
		printf("\n");
		printf("The child made a system call (%ld)(%ld)\n ", orig_eax,temp.regs.orig_rax);  
        printf("The child made a system call (%ld)(%ld)\n ", temp.regs.rax,temp.regs.rdi);  
        printf("The child made a system call (%ld)(%ld)\n ", temp.regs.rsi,temp.regs.rip);  
		ptrace(PTRACE_CONT, child, NULL, NULL);  
    }  
    return 0;  
}  
