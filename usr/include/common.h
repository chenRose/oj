#ifndef _COMMON_H_
#define _COMMON_H_
#include "log.h"
#include "judger.h"
#include "iniparser.h"

typedef struct HANDLER_LOAD_INFO_T
{
	char name[10];
	short total;			
	short pending;
}handler_load_info_t;
typedef struct CLIENT_MSG_T
{
	int report_type;					//new task (from php) or report of judger
	int info_num;						
	handler_load_info_t load[10];		//结构中只有10个handler_load_info_t结构的容量，而info_num为实际个数，如果info_num超过10，说明还可以再读取info_num-10个handler_load_info结构
}client_msg_t;
#endif
