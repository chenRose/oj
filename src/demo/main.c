#include<stdio.h>
#include<dlfcn.h>
#include<stdlib.h>
#define PATH "./handler/sample.so"

typedef void (*FUNC)();

void main()
{
	FUNC initial=NULL;
	FUNC start=NULL;

	void *handle;
	handle=dlopen(PATH,RTLD_LAZY);
	if(!handle)
	{
		printf("open .so %s error",PATH);
	}
        
        initial=dlsym(handle,"initial");
        initial();
	dlclose(handle);
	//initial();
        system("ls /");

}
