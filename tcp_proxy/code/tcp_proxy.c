#include "../header/tcp_proxy.h"

struct request_info
{
    char *serverIp;
    int serverPort;
    int clientFd;

    time_t openTime;
    time_t closeTime;
    time_t connTime;

    char sourceIp[16];
    int sourcePort;
    char desIP[16];
    int dstPort;
};

void proxy_process(int listenPort, char *serverIp, int serverPort)
{
    int listenFd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int res,nready,i,len,flag;
    pthread_t th;
    

    listenFd = create_listenfd(listenPort);

    printf("listenFd:%d\n",listenFd);
    int num=0;
    while(1){
        num++;
        struct request_info *requestInfo = (struct request_info*)malloc(sizeof(struct request_info));
        printf("%d process waiting for %d connection.....\n",getpid(),num);
        int clientFd = accept(listenFd,(struct sockaddr *)&client_addr, &client_addr_len);
        
        requestInfo->openTime = time(NULL);
        requestInfo->clientFd = clientFd;
        requestInfo->serverIp = serverIp;
        requestInfo->serverPort = serverPort;
        inet_ntop(AF_INET,&client_addr.sin_addr,requestInfo->sourceIp, sizeof(requestInfo->sourceIp));
        requestInfo->sourcePort = client_addr.sin_port;
        strcpy(requestInfo->desIP, serverIp);
        requestInfo->dstPort = serverPort;

        printf("this is %d client ...\n",num);
        int res = pthread_create(&th, NULL, handle_request, (void *)requestInfo);
    }
    close(listenFd);
}

void *handle_request(void *arg){
    struct request_info *requestInfo = (struct request_info*)arg;
    int len;
    char buffer[1024];
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(requestInfo->serverPort);
    server_addr.sin_addr.s_addr = inet_addr(requestInfo->serverIp);
    if( connect(server_fd, (struct sockaddr*)&server_addr,sizeof(server_addr)) < 0){
        printf("connect error\n");
        exit(0);
    }


    len = recv(requestInfo->clientFd,buffer,sizeof(buffer),0);
    if(len != 0)
    {
        printf("read:%s", buffer);
        send(server_fd, buffer, sizeof(buffer), 0);
        while ((len = recv(server_fd, buffer, sizeof(buffer), 0)) > 0)
        {

            send(requestInfo->clientFd, buffer, len, 0);
        }
        printf("over........\n");
    }
    //printf("server:%s",buffer);

    close(server_fd);
    close(requestInfo->clientFd);
    requestInfo->closeTime = time(NULL);
    requestInfo->connTime = requestInfo->closeTime - requestInfo->openTime;
    printf("sourceip: %s ; sourceport: %d\ndesip: %s ; desport: %d \n",requestInfo->sourceIp,requestInfo->sourcePort,requestInfo->desIP,requestInfo->dstPort);
    printf("opentime: %d ; closetime: %d ; conntime: %d ;\n",requestInfo->openTime, requestInfo->closeTime, requestInfo->connTime);
    free(requestInfo);
    pthread_exit(NULL);
}
