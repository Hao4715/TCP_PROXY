#include "../header/blog_sock.h"

int create_listenfd(int listen_port){

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    int res;
    if(listenfd == -1)
         return -1;
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(listen_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    res = bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(res == -1){
        printf("%d bind error\n",listen_port);
        exit(1);
    }
    listen(listenfd,20480);
    return listenfd;
}


int set_non_blocing(int fd)
{
    int opt;
    opt = fcntl(fd, F_GETFL);

    if(opt < 0)
    {
        printf("fcntl(fd, F_GETFL) error\n");
        return -1;
    }

    opt = opt | O_NONBLOCK;
    if(fcntl(fd, F_SETFL, opt) < 0)
    {
        printf("fcntl(fd, F_SETFL, opt) error\n");
        return -1;
    }
    return 0;
}