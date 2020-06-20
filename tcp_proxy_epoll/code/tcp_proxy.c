/*
* @brief     进程函数，逻辑主体
* @details   包含代理进程函数以及工作线程函数
*/
#include "../header/tcp_proxy.h"

#define OPEN_MAX 20480



pthread_mutex_t conn_mutex = PTHREAD_MUTEX_INITIALIZER;

void proxy_epoll(struct proxy *proxy_info)
{

    int client_num=0;
    int listen_fd,client_fd;
    int res = 0;
    int ep_fd,n_ready,n_i;
    int recv_len = 0;

    struct epoll_event tep,ep_event[OPEN_MAX];
    struct event *event_tmp,*event_info = (struct event *)malloc(sizeof(struct event));
    listen_fd = create_listenfd(proxy_info->listen_port);

    ep_fd = epoll_create(OPEN_MAX);
    if(ep_fd == -1)
    {
        printf("epoll create erroe\n");
        exit(1);
    }
    event_info->epoll_fd = listen_fd;
    event_info->dst_fd = 0;
    tep.events = EPOLLIN;
    tep.data.ptr = (void *)event_info;

    res = epoll_ctl(ep_fd,EPOLL_CTL_ADD,listen_fd,&tep);
    if(res == -1)
    {
        printf("epoll_ctl add root error\n");
        exit(1);
    }
    for( ; ; )
    {
        n_ready = epoll_wait(ep_fd, ep_event,OPEN_MAX,-1);
        if(n_ready == -1)
        {
            printf("epoll_wait error\n");
            exit(1);
        }
        for(n_i = 0; n_i < n_ready; ++n_i)
        {
            if(!(ep_event[n_i].events & EPOLLIN))
                continue;
            event_tmp = (struct event*)ep_event[n_i].data.ptr;
            if(event_tmp->epoll_fd == listen_fd)  //监听描述符有新请求
            {
            
                res = accept_and_conn(ep_fd, listen_fd, proxy_info);
                if(res == -1)
                {
                    printf("accept_and_conn error\n");
                    exit(1);
                }
            } 
            else
            {
                res = info_transmit(ep_fd, event_tmp);
                if(res == -1)
                {
                    printf("trans error\n");
                    exit(1);
                }
            }
            
        }
    }
}

int info_transmit(int ep_fd, struct event *trans_event)
{
    trans_event->buffer_len = recv(trans_event->epoll_fd,trans_event->buffer,sizeof(trans_event->buffer),0);
    if(trans_event->buffer_len > 0)
    {
        trans_event->buffer_len = send(trans_event->dst_fd, trans_event->buffer, trans_event->buffer_len, 0);
        //printf("%s\n",trans_event->buffer);
        return 1;
    }
    else if(trans_event->buffer_len == 0)
    {
        printf("client closed\n");
        epoll_ctl(ep_fd,EPOLL_CTL_DEL,trans_event->epoll_fd,NULL);
        epoll_ctl(ep_fd,EPOLL_CTL_DEL,trans_event->dst_fd,NULL);
        close(trans_event->epoll_fd);
        close(trans_event->dst_fd);
        return 1;
    }
    else
    {
        printf("recv error\n");
        return -1;
    }
}

int accept_and_conn(int ep_fd, int listen_fd, struct proxy *proxy_info)
{
    int client_fd,server_fd;
    int res;

    struct sockaddr_in client_addr, server_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    struct epoll_event client_event,server_event;
    struct event * client_event_info = (struct event *)malloc(sizeof(struct event));
    struct event * server_event_info = (struct event *)malloc(sizeof(struct event));

    client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_addr_len);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        printf("socket to server error\n");
        return -1;
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(proxy_info->server_port);
    server_addr.sin_addr.s_addr = inet_addr(proxy_info->server_ip);

    if (connect(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("connect error\n");
        return -1;
    }
    client_event_info->epoll_fd = client_fd;
    client_event_info->dst_fd = server_fd;
    client_event_info->buffer_len = 0;

    client_event.events = EPOLLIN;
    client_event.data.ptr = (void *)client_event_info;

    res = epoll_ctl(ep_fd, EPOLL_CTL_ADD, client_fd, &client_event);
    if (res == -1)
    {
        printf("epoll add client error\n");
        return -1;
    }

    server_event_info->epoll_fd = server_fd;
    server_event_info->dst_fd = client_fd;
    server_event_info->buffer_len = 0;

    server_event.events = EPOLLIN;
    server_event.data.ptr = (void *)server_event_info;
    res = epoll_ctl(ep_fd, EPOLL_CTL_ADD, server_fd, &server_event);
    if (res == -1)
    {
        printf("epoll add server error\n");
        return -1;
    }
    printf("conn ect\n");
    return 0;
}


