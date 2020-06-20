#include "../header/tcp_sock.h"

int create_listenfd(int listen_port){

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(listen_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    int res = bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(res == -1){
        printf("%d bind error\n",listen_port);
        exit(1);
    }
    listen(listenfd,20480);
    return listenfd;
}
