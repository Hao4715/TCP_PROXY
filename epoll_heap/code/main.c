#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define MAXLINE 5
#define BUFLEN 4095
#define LISTEN_SERV_PORT 8666
#define OPEN_MAX 5000



struct myevent_s{
    int fd;                      //listened fd
    int events;                  //evevt:EPOLLIN | EPOLLOUT | EPOLLERROR
    void *arg;                   //generic param
    void (*call_back)(int fd, int events, void *arg);//call back func
    int status;                  //wether on listen:  1 --->listened, 0  --->ont listened
    char buf[BUFLEN+1];
    int len;                     //length of buf
    long last_active;            //last active time
};

int g_epfd;                        //epoll tree root
struct myevent_s g_events[OPEN_MAX +1];

void recvData(int fd, int events, void *arg);
void sendData(int fd, int events, void *arg);

void eventSet(struct myevent_s *ev, int fd, void (*call_back)(int,int,void *), void *arg){
    ev->fd = fd;
    ev->call_back = call_back;
    ev->events = 0;
    ev->arg = arg;
    ev->status = 0;
    //memset(ev->buf,0,sizeof(ev->buf));
    //ev->len = 0;
    ev->last_active = time(NULL);
    return;
}

void eventAdd(int efd, int events, struct myevent_s *ev){
    struct epoll_event ep_ev = {0,{0}};
    int op;
    ep_ev.data.ptr = ev;
    ep_ev.events = ev->events = events;     //EPOLLIN || EPOLLOUT

    if(ev->status == 1) {
        op = EPOLL_CTL_MOD;
    } else {
        op = EPOLL_CTL_ADD;
        ev->status = 1;
    }

    if (epoll_ctl(efd, op,ev->fd, &ep_ev) < 0){       //add or modify
        printf("event add failed [fd=%d], events[%d]\n",ev->fd,events);
    }else {
        printf("event add OK [fd=%d],op=%d,  events[%0X]\n",ev->fd,op,events);
    }
    return;
}

void eventDel(int fd, struct myevent_s *ev){
    struct epoll_event epv = {0,{0}};
    if(ev->status != 1)
        return;
    epv.data.ptr = ev;
    ev->status = 0;
    epoll_ctl(fd,EPOLL_CTL_DEL,ev->fd,&epv);
    return;
}

void recvData(int fd, int events, void *arg){
    struct myevent_s *ev = (struct myevent_s*)arg;
    int recv_len;
    recv_len = recv(fd, ev->buf, sizeof(ev->buf),0);

    eventDel(g_epfd,ev);

    if(recv_len>0){
        ev->len = recv_len;
        ev->buf[recv_len] = '\0';
        printf("client [%d]: %s\n",fd,ev->buf);
        eventSet(ev,fd,sendData,ev);
        eventAdd(g_epfd,EPOLLOUT,ev);

    } else if(recv_len == 0) {
        close(ev->fd);
        printf("[fd = %d] [pos[%ld] closed\n",fd,ev-g_events);
    } else {
        close(ev->fd);
        printf("recv[fd = %d] error[%d]:%s\n",fd,errno, strerror(errno));
    }
    return;
}

void sendData(int fd, int events, void *arg){
    struct myevent_s *ev = (struct myevent_s*)arg;
    int send_len;
    send_len = send(fd, ev->buf, ev->len,0);
    if(send_len > 0){
        printf("send[fd=%d],[%d]:%s\n",fd,send_len,ev->buf);
        eventDel(g_epfd,ev);
        eventSet(ev,fd,recvData,ev);
        eventAdd(g_epfd,EPOLLIN,ev);
    } else {
        close(fd);
        eventDel(g_epfd,ev);
        printf("len =%d  send[fd = %d] error %s\n",send_len,fd,strerror(errno));
    }
    return;
}

void acceptConn(int listen_fd, int events, void *arg){
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);

    int client_fd,i;
    if((client_fd = accept(listen_fd,(struct sockaddr *)&client_addr,&len)) == -1){
        if(errno != EAGAIN && errno != EINTR){
            //todo:....
        }
        printf("%s: accept, %s\n",__func__,strerror(errno));
        return;
    }

    do {
        for(i = 0 ; i< OPEN_MAX;i++){
            if(g_events[i].status == 0)
                break;
        }
        if(i == OPEN_MAX) {
            printf("%s: max connect limit[%d]\n",__func__,OPEN_MAX);
            break;
        }
        int flag = 0;
        if((flag = fcntl(client_fd, F_SETFL, O_NONBLOCK)) < 0){
            printf("%s: fcntl nonblocking failed, %s\n",__func__,strerror(errno));
            break;
        }

        eventSet(&g_events[i],client_fd,recvData,&g_events[i]);
        eventAdd(g_epfd, EPOLLIN, &g_events[i]);
    }while(0);

    printf("new connect [%s:%d] [time:%ld],pos[%d]\n",
            inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port),
            g_events[i].last_active,i);
    return;
}

void initListenSocket(int efd, short port){

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(listenfd, F_SETFL, O_NONBLOCK);
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    eventSet(&g_events[OPEN_MAX], listenfd, acceptConn, &g_events[OPEN_MAX]);
    eventAdd(efd, EPOLLIN, &g_events[OPEN_MAX]);

    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    int res = bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(res == -1){
        perror("bind error\n");
    }
    listen(listenfd,20);
    return;
}

int main(){

    unsigned short port = LISTEN_SERV_PORT;
    g_epfd = epoll_create(OPEN_MAX+1);
    if(g_epfd <= 0)
        printf("create efd in %s err %s\n",__func__,strerror(errno));
    
    initListenSocket(g_epfd,port);

    struct epoll_event events[OPEN_MAX +1];
    int checkops = 0, i,nready;

    while (1){
        /*long now = time(NULL);
        for (i = 0; i < 100; i++, checkops++){
            if (checkops == OPEN_MAX)
                checkops = 0;
            if (g_events[checkops].status != 1)
                continue;

            long duration = now - g_events[checkops].last_active;

            if (duration >= 60){
                close(g_events[checkops].fd);
                printf("[fd=%d] timeout\n", g_events[checkops].fd);
                eventDel(g_epfd, &g_events[checkops]);
            }
        }*/

        printf("***wait****\n");

        nready = epoll_wait(g_epfd, events, OPEN_MAX + 1, -1);
        printf("ready num : %d\n",nready);
        if(nready < 0){
            printf("epoll_wait error\n");
            break;
        }
        for(i=0;i<nready;i++){
            struct myevent_s *ev = (struct myevent_s*)events[i].data.ptr;

            if((events[i].events & EPOLLIN) && (ev->events & EPOLLIN)){ //ready to read
                ev->call_back(ev->fd,events[i].events,ev->arg);
            }
            if((events[i].events & EPOLLOUT) && (ev->events & EPOLLOUT)){ //ready to read
                ev->call_back(ev->fd,events[i].events,ev->arg);
            }
        }
    }
    return 0;
}