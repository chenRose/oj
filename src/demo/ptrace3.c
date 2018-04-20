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
		struct user_regs_struct  temp;
		memset(&temp,0,sizeof(temp));
		orig_eax = ptrace(PTRACE_GETREGS,   
                          child,
						 120,   
                          &temp);  
       printf("The child made a system call (%ld)(%ld)\n ", temp.orig_rax,temp.orig_rax);  
		ptrace(PTRACE_CONT, child, NULL, NULL);  
    }  
    return 0;  
}  
