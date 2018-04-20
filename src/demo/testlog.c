#include "../../include/log.h"

void main(int argc,char *argv[])
{
	if(argc!=2)
		exit(1);
	
	log_error_s(argv[1],"hello %s world %s\n","chen","shuyu");
}
