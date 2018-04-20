#include <sys/ptrace.h>
#include<stdlib.h>
#include <sys/wait.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <stdio.h>
int main(){
    pid_t child;
    long orig_rax;
    int status;
    int iscalling=0;
    struct user_regs_struct regs;

    child = fork();
        if(child==0){
          ptrace(PTRACE_TRACEME,0,NULL,NULL);
	//freopen("/dev/null","w",stdout);
//      execl("/data/oj/20180402/2014150216/1/0001Ur7GN5","a.out",NULL);
	execl("./a.out","a.out",NULL);
    }else{
          while(1){
        wait(&status);
                if(WIFEXITED(status))
                    break;
                orig_rax=ptrace(PTRACE_PEEKUSER,child,8*ORIG_RAX,NULL);
                //if(orig_rax == SYS_open||orig_rax==SYS_read||orig_rax==SYS_write){
if(orig_rax==59){ptrace(PTRACE_SYSCALL,child,NULL,NULL);continue;         }         
if(1){  ptrace(PTRACE_GETREGS,child,NULL,&regs);
            if(!iscalling){
                iscalling =1;
                printf("%d call with %lld, %lld, %lld\n",regs.orig_rax,regs.rdi,regs.rsi,regs.rdx);
                        } else{
                               printf("%d return 0x%x\n",regs.orig_rax,regs.rax);
                               iscalling = 0;
                        }                                  


                }
            ptrace(PTRACE_SYSCALL,child,NULL,NULL);
          }
       }
          return 0;
}
