#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>

#define MAXLINE 5
#define LISTEN_SERV_PORT 8666
#define OPEN_MAX 5000

struct myevent{
    int fd;
};

int create_listenfd(){

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(LISTEN_SERV_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    int res = bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(res == -1){
        perror("bind error\n");
    }
    listen(listenfd,128);
    return listenfd;
}

int main(){
    int listen_fd, conn_fd,sock_fd,ep_fd;
    int res,nready,i,num = 0,len,flag;
    char buf[MAXLINE],str[INET_ADDRSTRLEN];
    socklen_t client_len;
    struct sockaddr_in client_addr;
    struct epoll_event event_tmp, events[OPEN_MAX];
    listen_fd = create_listenfd();

    printf("listen_fd:%d\n",listen_fd);

    ep_fd = epoll_create(OPEN_MAX);
    if(ep_fd <= 0){
        printf("epoll_create error\n");
    }

    struct myevents *myevents_tmp = (struct myevents*)malloc(sizeof(struct myevent));
    event_tmp.events = EPOLLIN;
    event_tmp.data.fd = listen_fd;
    //event_tmp.data.ptr = NULL;
    res = epoll_ctl(ep_fd, EPOLL_CTL_ADD, listen_fd, &event_tmp);
    if(res == -1){
        printf("epoll_ctl error\n");
        exit(1);
    }
    while(1){

        printf("***wait****\n");

        nready = epoll_wait(ep_fd,events,OPEN_MAX,-1);
        printf("ready num : %d\n",nready);
        if(nready == -1){
            printf("epoll_wait error\n");
            exit(-1);
        }
        for(i=0;i<nready;i++){
            if(!(events[i].events & EPOLLIN))
                continue;
            if(events[i].data.fd == listen_fd){
                client_len = sizeof(client_addr);
                conn_fd = accept(listen_fd,(struct sockaddr*)&client_addr, &client_len);
                
                printf("received from %s at port %d\n",
                        inet_ntop(AF_INET, &client_addr.sin_addr,str, sizeof(str)),
                        ntohs(client_addr.sin_port));
                printf("client_fd %d-----client %d",conn_fd,++num);

                //set connfd to nonblock_IO
                flag = fcntl(conn_fd, F_GETFL);
                flag |= O_NONBLOCK;
                fcntl(conn_fd,F_SETFL, flag);


                event_tmp.events = EPOLLIN | EPOLLET;
                event_tmp.data.fd = conn_fd;
                res = epoll_ctl(ep_fd,EPOLL_CTL_ADD,conn_fd,&event_tmp);
                if(res == -1){
                    printf("epoll_ctl add error\n");
                    exit(-1);
                }
            } else {
                sock_fd = events[i].data.fd;
                bzero(buf,MAXLINE);
                len = read(sock_fd,buf, MAXLINE);
                if(len == 0){          //client closed connection
                    res = epoll_ctl(ep_fd,EPOLL_CTL_DEL,sock_fd,NULL);
                    if(res == -1){
                        printf("len == 0 epoll delete error\n");
                        exit(-1);
                    }
                    close(sock_fd);
                    printf("client[%d] closed connection\n",sock_fd);
                }
                else if (len <0){
                    perror("read n < 0 error\n");
                    res = epoll_ctl(ep_fd,EPOLL_CTL_DEL,sock_fd,NULL);
                    if(res == -1){
                        printf("len == 0 epoll delete error\n");
                        exit(-1);
                    }
                    close(sock_fd);
                }else {
                    printf("meddege:%s\n",buf);
                    while((len = read(conn_fd,buf,MAXLINE)) > 0){
                        printf("%d meddege old:%s\n",len,buf);
                    }
                }
            }
        }
    }
    return 0;
}
