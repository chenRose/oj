#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/reg.h>
#include <sys/syscall.h>
#include <sys/user.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#define long_size sizeof(long)

void reverse(char * str)
{
    int i,j;
    char temp;
    for(i=0,j=strlen(str)-2;i<=j;++i,--j){
          temp=str[i];
          str[i]=str[j];
          str[j]=temp;
    }
}


void getdata(pid_t child,long addr,char * str,int len){
  char * laddr;
  int i,j;
  union u{
    long val;
        char chars[long_size];
  } data;
  i=0;
  j=len/long_size;
  laddr=str;
  while(i<j){
   data.val=ptrace(PTRACE_PEEKDATA,child,addr+i*long_size,NULL);
   if(data.val == -1){
     if(errno){
        printf("READ error: %s\n",strerror(errno));
     }
   }
     memcpy(laddr,data.chars,long_size);
     ++i;
     laddr +=long_size;
  };
  j=len % long_size;
  if(j!=0){
     data.val=ptrace(PTRACE_PEEKDATA,child,addr+i*long_size,NULL);
     memcpy(laddr,data.chars,j);
  }
   str[len]='\0';
}

void putdata(pid_t child,long addr,char * str,int len){
    char * laddr;
        int i,j;
        union u{
          long val;
          char chars[long_size];
       } data;
       i=0;
       j=len /long_size;
       laddr=str;
       while(i<j){
           memcpy(data.chars,laddr,long_size);
           ptrace(PTRACE_POKEDATA,child,addr +i*long_size,data.val);
           ++i;
           laddr+=long_size;
     }
     j=len%long_size;
     if(j!=0){   
  memcpy(data.chars,laddr,j);
           ptrace(PTRACE_POKEDATA,child,addr +i*long_size,data.val);
     }

}

int main(){
    pid_t child;
        int status;
        struct user_regs_struct regs;
        child =fork();
        if(child ==0){
          ptrace(PTRACE_TRACEME,0,NULL,NULL);
          execl("/bin/ls","ls",NULL);
       }else{
           long orig_eax;

           char *str,*laddr;
           int toggle =0;
           while(1){
             wait(&status);
             if(WIFEXITED(status))
                 break;
             orig_eax = ptrace(PTRACE_PEEKUSER,child,8*ORIG_RAX,NULL);
             if(orig_eax == SYS_write){
               if(toggle == 0){
                  toggle =1;
                 ptrace(PTRACE_GETREGS,child,NULL,&regs);

                 str=(char * )calloc((regs.rdx+1),sizeof(char));
                 getdata(child,regs.rsi,str,regs.rdx);
                 reverse(str);
                 putdata(child,regs.rsi,str,regs.rdx);
              }else{
              toggle =0;
               }
            }

         ptrace(PTRACE_SYSCALL,child,NULL,NULL);
          }
       }
      return 0;
}


