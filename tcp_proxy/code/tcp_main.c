#include <sys/types.h>
#include <sys/wait.h>
#include "../header/tcp_sock.h"
#include "../header/tcp_proxy.h"
#include "../header/tcp_conf.h"
#include "../header/tcp_statistics.h"


int main(){ 
    int i,proxy_num=0;
    struct proxy* proxy_info = get_conf_info(&proxy_num);
    struct statistics * statistics_info = statisticsInit();
    int pid[proxy_num],pid_statistics;;
    int accessLog = open("access.log",O_RDWR | O_APPEND);

    if((pid_statistics = fork()) == 0)
    {
        proxy_show_statisitcs(statistics_info);
        exit(1);
    }
    for (i = 0; i < proxy_num; i++)
    {
        printf("proxy %d : \n     port:%d ;  ip:%s ; port:%d;\n",i,proxy_info[i].listen_port,proxy_info[i].server_ip,proxy_info[i].server_port);
        if((pid[i] = fork()) == 0)
        {
            proxy_process(proxy_info[i].listen_port,proxy_info[i].server_ip,proxy_info[i].server_port,accessLog,statistics_info);
            printf("tuichule:%d\n",getpid());
            exit(1);
        }
    }
    sleep(1000);
    int state;
    wait(&state);
    printf("over\n");
    return 0;
}
