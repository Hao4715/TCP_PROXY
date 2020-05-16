#ifndef _TCP_STATISTICS_H
#define _TCP_STATISTICS_H
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <pthread.h>
#include <time.h>
struct statistics
{
    //CPS:connections_per_seconds
    unsigned int client_proxy_CPS;
    //pthread_mutex_t client_proxy_CPS_mutex;
    unsigned int client_proxy_connections_now;
    pthread_mutex_t client_proxy_connections_now_mutex;

    unsigned int client_proxy_connections_all;
    pthread_mutex_t client_proxy_connections_mutex;

    unsigned int client_proxy_connections_finished;
    //pthread_mutex_t client_proxy_connections_finished_mutex;

    unsigned long client_to_proxy_data;
    //pthread_mutex_t client_to_proxy_data_mutex;

    unsigned long proxy_to_client_data;
    //pthread_mutex_t proxy_to_client_data_mutex;;


    unsigned int proxy_server_CPS;
    //pthread_mutex_t proxy_server_CPS_mutex;
    unsigned int proxy_server_connections_now;
    pthread_mutex_t proxy_server_connections_now_mutex;

    unsigned int proxy_server_connections_all;
    pthread_mutex_t proxy_server_connections_mutex;

    unsigned int proxy_server_connections_finished;
    pthread_mutex_t connections_finished_mutex;

    unsigned long server_to_proxy_data;
    pthread_mutex_t response_data_mutex;

    unsigned long proxy_to_server_data;
    pthread_mutex_t request_data_mutex;

    pthread_mutexattr_t mutex_attr;
};

struct statistics * statisticsInit();
void proxy_show_statisitcs(struct statistics *statistics_info);

#endif