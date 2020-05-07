#include <time.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <fcntl.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>


#define MAXLINE 1024
#define SA struct sockaddr
#define LPORT 7777

int main(void)
{
    int lfd;
    int cfd;
    struct sockaddr_in6 servAddr,clenAddr;
    char buff[MAXLINE];
    time_t ticks;
    lfd = socket(PF_INET6, SOCK_STREAM, 0);
    memset(&servAddr, 0, sizeof(servAddr));

    servAddr.sin6_family = AF_INET6;
    servAddr.sin6_addr = in6addr_any;
    servAddr.sin6_port = htons(LPORT);
    bind(lfd, (SA *)&servAddr, sizeof(servAddr));
    listen(lfd, 5);
    char buf[MAXLINE];
    socklen_t len;
    for (;;)
    {
        printf("wait\n");
        len = sizeof(clenAddr);
        cfd = accept(lfd, (SA *)&clenAddr, &len);
        printf("ip:%s port: %d \n",inet_ntop(AF_INET6,&clenAddr.sin6_addr,buf,sizeof(buf)),clenAddr.sin6_port);
        ticks = time(NULL);
        snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));
        write(cfd, buff, strlen(buff));
        
        close(cfd);
    }
    return 0;
}
