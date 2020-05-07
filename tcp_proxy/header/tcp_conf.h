#ifndef _TCP_CONF_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

struct proxy
{
    int listen_port;
    char server_ip[16];
    int server_port;
};

struct proxy* get_conf_info(int *proxyNum);
void block_read(FILE *file, int *blockNum,int *ch,struct proxy *proxyInfo);
struct proxy * new_proxy(struct proxy* oldProxyInfo,int *proxyNum,int blockNum);
int is_ip_legal(char *ip,int len);
#endif