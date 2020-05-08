#include "../header/tcp_proxy.h"

struct request_info
{
    int clientFd;

    time_t openTime;
    time_t closeTime;
    time_t connTime;

    char clientIp[16];
    int clientPort;
    int listenPort;
    char serverIp[16];
    int serverPort;

    int accessLog;
};

void proxy_process(int listenPort, char *serverIp, int serverPort,int accessLog)
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

        strcpy(requestInfo->serverIp, serverIp);
        requestInfo->serverPort = serverPort;
        requestInfo->listenPort = listenPort;
        inet_ntop(AF_INET,&client_addr.sin_addr,requestInfo->clientIp, sizeof(requestInfo->clientIp));
        requestInfo->clientPort = client_addr.sin_port;

        requestInfo->accessLog = accessLog;

        printf("this is %d client ...\n",num);
        int res = pthread_create(&th, NULL, handle_request, (void *)requestInfo);
    }
    close(listenFd);
}

void *handle_request(void *arg){
    char *logFormat = "%s - - [ %s ] \"%s\" [%s:%d]--[%s:%d]--[%s:%d] request_length:%d  response_length:%d\n";
    char log[1024];
    struct request_info *requestInfo = (struct request_info*)arg;
    int request_len,len,response_len=0;
    char bufferRequest[1024],bufferResponse[1024];
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(requestInfo->serverPort);
    server_addr.sin_addr.s_addr = inet_addr(requestInfo->serverIp);
    if( connect(server_fd, (struct sockaddr*)&server_addr,sizeof(server_addr)) < 0){
        printf("connect error\n");
        exit(0);
    }


    request_len = recv(requestInfo->clientFd,bufferRequest,sizeof(bufferRequest),0);
    if(request_len != 0)
    {
        printf("read:%s", bufferRequest);
        send(server_fd, bufferRequest, sizeof(bufferRequest), 0);
        while ((len = recv(server_fd, bufferResponse, sizeof(bufferResponse), 0)) > 0)
        {
            response_len += len;
            send(requestInfo->clientFd, bufferResponse, len, 0);
        }
        printf("over........\n");
    }
    //printf("server:%s",buffer);

    char buf[64];
    memset(buf,0,sizeof(buf));
    if(flock(requestInfo->accessLog,LOCK_EX) < 0)
    {
        printf("lockerrorrrrrrrr\n");
        exit(0);
    }
    /*lseek(requestInfo->accessLog,0,SEEK_SET);
    int l = read(requestInfo->accessLog,buf,sizeof(buf));
    printf("read buf: %s\n",buf);
    int i,sum=0;
    buf[l] = '\0';
    sum = atoi(buf);
    ++sum;
    sprintf(buf,"%d",sum);
    lseek(requestInfo->accessLog,0,SEEK_SET);*/

    sprintf(log,logFormat,"192.168.1.5",ctime(&requestInfo->openTime),bufferRequest,requestInfo->clientIp,requestInfo->clientPort,"192.168.1.5",requestInfo->listenPort
                ,requestInfo->serverIp,requestInfo->serverPort,request_len,response_len);
    write(requestInfo->accessLog,log,strlen(log));
    int ret = flock(requestInfo->accessLog,LOCK_UN);
    if(ret == -1)
    {
        printf("unlockerror\n");
        exit(0);
    }
    //printf("sum: %d , sumbuf: %s\n",sum,buf);
    

    close(server_fd);
    close(requestInfo->clientFd);
    requestInfo->closeTime = time(NULL);
    requestInfo->connTime = requestInfo->closeTime - requestInfo->openTime;
    printf("sourceip: %s ; sourceport: %d\ndesip: %s ; desport: %d \n",requestInfo->clientIp,requestInfo->clientPort,requestInfo->serverIp,requestInfo->serverPort);
    printf("opentime: %d ; closetime: %d ; conntime: %d ;\n",requestInfo->openTime, requestInfo->closeTime, requestInfo->connTime);
    free(requestInfo);
    pthread_exit(NULL);
}
