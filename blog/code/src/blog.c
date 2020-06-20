#include <stdio.h>
#include <sys/epoll.h>
#include <errno.h>
#include "../header/blog_sock.h"
#include "../header/blog_request.h"

#define OPEN_MAX 2048
#define MAXLINE 1024

struct my_event
{
    int fd;
    int file_fd;
};

int accept_conn(int ep_fd, int listen_fd)
{
    int res;
    int client_fd;

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    struct epoll_event ep_event;
    struct my_event *event = (struct my_event *)malloc(sizeof(struct my_event));
    client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    printf("client_fd:%d\n",client_fd);
    if(client_fd == -1)
        return -1;

    event->fd = client_fd;
    event->file_fd = -1;
    ep_event.events = EPOLLIN;
    ep_event.data.ptr = (void *)event;
    res = epoll_ctl(ep_fd, EPOLL_CTL_ADD, client_fd, &ep_event);
    if(res == -1)
    {
        printf("epoll_ctl add error\n");
        return -1;
    }
    return 1;
}

int recv_request(int ep_fd, struct my_event *event_temp)
{

    int res;
    int recv_len; 
    char buffer_req[MAXLINE];
    bzero(buffer_req,sizeof(buffer_req));

    struct epoll_event ep_event;

    recv_len = recv(event_temp->fd,buffer_req,sizeof(buffer_req),0);
    if(recv_len > 0)
    {
        //获取文件描述符
        if((event_temp->file_fd = get_file_fd(buffer_req)) == -1)
        {
            printf("recv_request open file error\n");
            return -1;
        }
        ep_event.events = EPOLLOUT;
        ep_event.data.ptr = (void *)event_temp;
        res = epoll_ctl(ep_fd, EPOLL_CTL_MOD, event_temp->fd, &ep_event);
        if (res == -1)
        {
            printf("epoll_ctl modify error\n");
            return -1;
        }
    }
    else if (recv_len == 0)
    {
        printf("closed\n");
        //客户端请求断开连接
        res = epoll_ctl(ep_fd,EPOLL_CTL_DEL,event_temp->fd,NULL);
        close(event_temp->fd);
        if(res == -1)
        {
            printf("client closed and dele error\n");
            return -1;
        }
    }
    else
    {
        //出错
        res = epoll_ctl(ep_fd,EPOLL_CTL_DEL,event_temp->fd,NULL);
        printf("error:%s\n",strerror(errno));
        close(event_temp->fd);
        if(res == -1)
        {
            printf("client recv error or dele error\n");
            return -1;
        }

    }
}

int send_response(int ep_fd, struct my_event *event_temp)
{
    char buffer_res[MAXLINE];
    int read_len = 0,res;
    read_len = read(event_temp->file_fd, buffer_res, sizeof(buffer_res));
    if(read_len < 0)
    {
        printf("read file error\n");
        return -1;
    }
    else if (read_len == 0)
    {
        printf("send over\n");
        res = epoll_ctl(ep_fd,EPOLL_CTL_DEL,event_temp->fd,NULL);
        close(event_temp->fd);
        close(event_temp->file_fd);
        free(event_temp);
        event_temp = NULL;
        return 1;
    }
    else
    {
        res = send(event_temp->fd,buffer_res,read_len,0);
        if(res == -1)
        {
            printf("send error\n");
            return -1;
        }
    }
    
}
void blog_handle()
{
    int listen_port = 8666;
    int listen_fd,client_fd;
    int res = 0;
    int ep_fd,n_ready,n_i;


    struct epoll_event temp,ep_events[OPEN_MAX];
    struct my_event *event_temp, event_info;
    listen_fd = create_listenfd(listen_port);

    if(listen_fd == -1)
    {
        printf("create listen_fd error\n");
        exit(1);
    }
    ep_fd = epoll_create(OPEN_MAX);
    if(ep_fd == -1)
    {
        printf("epoll create error\n");
        exit(1);
    }

    event_info.fd = listen_fd;
    event_info.file_fd = -1;
    temp.events = EPOLLIN;
    temp.data.ptr = (void *)&event_info;

    res = epoll_ctl(ep_fd, EPOLL_CTL_ADD, listen_fd, &temp);
    if(res == -1)
    {
        printf("create epoll_fd error\n");
        exit(1);
    }

    for( ; ; )
    {
        n_ready = epoll_wait(ep_fd,ep_events,OPEN_MAX,-1);
        if(n_ready == -1)
        {
            printf("epoll wait error\n");
            exit(1);
        }
        for (n_i = 0; n_i < n_ready; ++n_i)
        {
            event_temp = (struct my_event *)ep_events[n_i].data.ptr;

            if(event_temp->fd == listen_fd)
            {
                res = accept_conn(ep_fd, listen_fd);
                if (res == -1)
                {
                    printf("add client_fd error\n");
                    exit(1);
                }
            }
            else if (ep_events[n_i].events & EPOLLIN)
            {
                res = recv_request(ep_fd,event_temp);
                if(res == -1)
                {
                    printf("recv_request error\n");
                    exit(1);
                }
            }
            else if (ep_events[n_i].events & EPOLLOUT)
            {
                res = send_response(ep_fd, event_temp);
            }
            else
            {
                printf("wrong event\n");
            }
        }
        
    }

}

int main()
{
    chdir("..");
    printf("hello blog\n");
    blog_handle();
    return 0;
}



