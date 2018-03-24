#include<stdio.h>
#include<stdlib.h>
#include<syslog.h>
void main()
{
    openlog("syslog_test", LOG_PID, LOG_DAEMON);

//    syslog(LOG_EMERG, "system is unusable");
//    syslog(LOG_ALERT, "action must be taken immediately");
    syslog(LOG_CRIT, "critical conditions,%d",11);
    syslog(LOG_ERR, "error conditions");
    syslog(LOG_WARNING, "warning conditions");
    syslog(LOG_NOTICE, "normal, but significant, condition");
    syslog(LOG_INFO, "informational message");
    syslog(LOG_DEBUG, "debug-level message");

    closelog();

}
