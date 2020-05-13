#include <sys/types.h>
#include <sys/wait.h>
#include "../header/tcp_sock.h"
#include "../header/tcp_proxy.h"
#include "../header/tcp_conf.h"

struct statistics_info
{
    unsigned int connections_per_seconds;
    unsigned int  connections_all;
    unsigned int  connections_finished;

    unsigned long data_all;
    unsigned long data_client;
    unsigned long data_server;
};

int main(){ 
    int i,proxy_num=0;
    struct proxy* proxy_info = get_conf_info(&proxy_num);
    int pid[proxy_num];
    int accessLog = open("access.log",O_RDWR | O_APPEND);
    for (i = 0; i < proxy_num; i++)
    {
        printf("proxy %d : \n     port:%d ;  ip:%s ; port:%d;\n",i,proxy_info[i].listen_port,proxy_info[i].server_ip,proxy_info[i].server_port);
        if((pid[i] = fork()) == 0)
        {
            proxy_process(proxy_info[i].listen_port,proxy_info[i].server_ip,proxy_info[i].server_port,accessLog);
            exit(1);
        }
    }
    int state;
    wait(&state);
    printf("over\n");
    return 0;
}
