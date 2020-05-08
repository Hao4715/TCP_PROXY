#include "../header/tcp_sock.h"

int create_listenfd(int listenPort){

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(listenPort);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    int res = bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(res == -1){
        printf("%d bind error\n",listenPort);
        exit(1);
    }
    listen(listenfd,128);
    return listenfd;
}
