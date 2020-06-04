#ifndef _TCP_PROXY_H
#define _TCP_PROXY_H
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/file.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <pthread.h>
#include "tcp_statistics.h"

struct request_info         //请求信息结构体
{
    int client_fd;          //客户端监听描述符

    time_t open_time;       //客户端连接时间
    time_t close_time;      //客户端关闭时间
    time_t conn_time;       //连接持续时间

    char client_ip[16];     //客户端IP
    int client_port;        //客户端端口
    int listen_port;        //代理程序监听端口
    char server_ip[16];     //服务器ip  
    int server_port;        //服务器端口

    int access_log;         //日志文件
    struct statistics *statistics_info;  //输出统计信息结构体
};


void proxy_process(int listen_port, char *server_ip, int server_port,int access_log,struct statistics * statistics_info);
void *handle_request(void *arg);
#endif