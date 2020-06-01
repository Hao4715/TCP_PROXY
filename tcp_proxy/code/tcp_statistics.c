#include "../header/tcp_statistics.h"

struct statistics * statisticsInit()
{
    int fd = open("/dev/zero",O_RDWR);
    //struct statistics * statistics_info = (struct statistics *)malloc(sizeof(struct statistics));
    struct statistics * statistics_info = mmap(NULL,sizeof(struct statistics),PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);

    //struct th *th_info = mmap(NULL,sizeof(struct th),PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);

    statistics_info->client_proxy_CPS = 0;
    statistics_info->client_proxy_connections_now = 0;
    statistics_info->client_proxy_connections_all = 0;
    statistics_info->client_proxy_connections_finished = 0;
    statistics_info->client_to_proxy_data = 0;
    statistics_info->proxy_to_client_data = 0;

    statistics_info->proxy_server_CPS = 0;
    statistics_info->proxy_server_connections_now = 0;
    statistics_info->proxy_server_connections_all = 0;
    statistics_info->proxy_server_connections_finished = 0;
    statistics_info->server_to_proxy_data = 0;
    statistics_info->proxy_to_server_data = 0;

    pthread_mutexattr_init(&(statistics_info->mutex_attr));
    pthread_mutexattr_setpshared(&(statistics_info->mutex_attr),PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(statistics_info->client_proxy_connections_mutex),&(statistics_info->mutex_attr));
    pthread_mutex_init(&(statistics_info->proxy_server_connections_mutex),&(statistics_info->mutex_attr));
    pthread_mutex_init(&(statistics_info->request_data_mutex),&(statistics_info->mutex_attr));
    pthread_mutex_init(&(statistics_info->response_data_mutex),&(statistics_info->mutex_attr));
    pthread_mutex_init(&(statistics_info->connections_finished_mutex),&(statistics_info->mutex_attr));
    pthread_mutex_init(&(statistics_info->client_proxy_connections_now_mutex),&(statistics_info->mutex_attr));
    pthread_mutex_init(&(statistics_info->proxy_server_connections_now_mutex),&(statistics_info->mutex_attr));

    return statistics_info;
}

void proxy_show_statisitcs(struct statistics *statistics_info)
{
    int i = 0;
    unsigned int count = 0;
    while (1)
    {
        if(i == 0)
        {
            printf("                               client ------ > proxy:                               ||                               proxy ------ > server:\n");
            printf(" 每秒新建连接数 | 当前并发连接数 |  已完成连接数  |       in       |       out      || 每秒新建连接数 | 当前并发连接数 |  已完成连接数  |       in       |       out\n");
        }
        printf("%-16u|%-16u|%-16u|%-16ld|%-16ld||%-16u|%-16u|%-16u|%-16ld|%-16ld \n",
        statistics_info->client_proxy_connections_all, statistics_info->client_proxy_connections_now, statistics_info->client_proxy_connections_finished,
        statistics_info->client_to_proxy_data, statistics_info->proxy_to_client_data,
        statistics_info->proxy_server_connections_all, statistics_info->proxy_server_connections_now, statistics_info->proxy_server_connections_finished,
        statistics_info->server_to_proxy_data, statistics_info->proxy_to_server_data);
        pthread_mutex_lock(&(statistics_info->client_proxy_connections_mutex));
        statistics_info->client_proxy_CPS = 0;
        pthread_mutex_unlock(&(statistics_info->client_proxy_connections_mutex));
        pthread_mutex_lock(&(statistics_info->proxy_server_connections_mutex));
        statistics_info->proxy_server_CPS = 0;
        pthread_mutex_unlock(&(statistics_info->proxy_server_connections_mutex));
        i = i < 10 ? i+1 : 0;
        sleep(1);
    }
}