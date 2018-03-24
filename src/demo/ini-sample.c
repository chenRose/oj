#include <stdio.h>
#include <string.h>
#include "iniparser.h"

int main(int argc, char *argv[]){
    if(argc < 2){
        fprintf(stderr, "usage: %s <ini.file>\n", argv[0]);
        return 1;
    }

    char *file = argv[1];
    dictionary *ini = iniparser_load(file);

    printf("[server]\n");
    printf("host = %s\n", iniparser_getstring(ini, "server:host", NULL));
    printf("port = %d\n", iniparser_getint(ini, "server:port", 0));
    printf("pass = %s\n", iniparser_getstring(ini, "server:pass", NULL));
    printf("ssl = %d\n", iniparser_getboolean(ini, "server:ssl", 0));
    printf("tcp = %d\n", strcmp(iniparser_getstring(ini, "server:tcp", NULL), "enable") ? 0:1);
    printf("udp = %d\n", strcmp(iniparser_getstring(ini, "server:udp", NULL), "enable") ? 0:1);

    printf("\n[client]\n");
    printf("bind = %s\n", iniparser_getstring(ini, "client:bind", NULL));
    printf("listen = %d\n", iniparser_getint(ini, "client:listen", 0));
    printf("verbose = %d\n", iniparser_getboolean(ini, "client:verbose", 0));
    printf("daemon = %d\n", iniparser_getboolean(ini, "client:daemon", 0));

    iniparser_freedict(ini);
    return 0;
}
