#include <sys/types.h>
#include <sys/wait.h>
#include "../header/tcp_sock.h"
#include "../header/tcp_proxy.h"
#include "../header/tcp_conf.h"


int main(){ 
    int i,proxyNum=0;
    struct proxy* proxyInfo = get_conf_info(&proxyNum);
    int pid[proxyNum];
    int accessLog = open("access.log",O_RDWR | O_APPEND);
    for (i = 0; i < proxyNum; i++)
    {
        printf("proxy %d : \n     port:%d ;  ip:%s ; port:%d;\n",i,proxyInfo[i].listen_port,proxyInfo[i].server_ip,proxyInfo[i].server_port);
        if((pid[i] = fork()) == 0)
        {
            proxy_process(proxyInfo[i].listen_port,proxyInfo[i].server_ip,proxyInfo[i].server_port,accessLog);
            exit(1);
        }
    }
    int state;
    wait(&state);
    printf("over\n");
    return 0;
}
