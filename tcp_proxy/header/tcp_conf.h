#ifndef _TCP_CONF_H
#define _TCP_CONF_H
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

struct proxy* get_conf_info(int *proxy_num);
void block_read(FILE *file, int *block_num,int *ch,struct proxy *proxy_info);
struct proxy * new_proxy(struct proxy* old_proxy_info,int *proxy_num,int block_num);
int is_ip_legal(char *ip,int len);
#endif