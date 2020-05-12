#include "../header/tcp_proxy.h"

struct request_info
{
    int client_fd;

    time_t open_time;
    time_t close_time;
    time_t conn_time;

    char client_ip[16];
    int client_port;
    int listen_port;
    char server_ip[16];
    int server_port;

    int access_log;
};

void proxy_process(int listen_port, char *server_ip, int server_port,int access_log)
{
    int listen_fd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int res,nready,i,len,flag;
    pthread_t th;
    

    listen_fd = create_listenfd(listen_port);

    printf("listenFd:%d\n",listen_fd);
    int num=0;
    while(1){
        num++;
        struct request_info *request_info = (struct request_info*)malloc(sizeof(struct request_info));
        printf("%d process waiting for %d connection.....\n",getpid(),num);
        int client_fd = accept(listen_fd,(struct sockaddr *)&client_addr, &client_addr_len);
        
        request_info->open_time = time(NULL);
        request_info->client_fd = client_fd;

        strcpy(request_info->server_ip, server_ip);
        request_info->server_port = server_port;
        request_info->listen_port = listen_port;
        inet_ntop(AF_INET,&client_addr.sin_addr,request_info->client_ip, sizeof(request_info->client_ip));
        request_info->client_port = client_addr.sin_port;

        request_info->access_log = access_log;

        printf("this is %d client ...\n",num);
        int res = pthread_create(&th, NULL, handle_request, (void *)request_info);
    }
    close(listen_fd);
}

void *handle_request(void *arg){
    char *log_format = "%s - - [ %s ] \"%s\" [%s:%d]--[%s:%d]--[%s:%d] request_length:%d  response_length:%d\n";
    char log[1024];
    struct request_info *request_info = (struct request_info*)arg;
    int request_len,len,response_len=0;
    char buffer_request[1024],buffer_response[1024];
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(request_info->server_port);
    server_addr.sin_addr.s_addr = inet_addr(request_info->server_ip);
    if( connect(server_fd, (struct sockaddr*)&server_addr,sizeof(server_addr)) < 0){
        printf("connect error\n");
        exit(0);
    }


    request_len = recv(request_info->client_fd,buffer_request,sizeof(buffer_request),0);
    if(request_len != 0)
    {
        printf("read:%s", buffer_request);
        send(server_fd, buffer_request, sizeof(buffer_request), 0);
        while ((len = recv(server_fd, buffer_response, sizeof(buffer_response), 0)) > 0)
        {
            response_len += len;
            send(request_info->client_fd, buffer_response, len, 0);
        }
        printf("over........\n");
    }
    //printf("server:%s",buffer);

    char buf[64];
    memset(buf,0,sizeof(buf));
    if(flock(request_info->access_log,LOCK_EX) < 0)
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

    sprintf(log,log_format,"192.168.1.5",ctime(&request_info->open_time),buffer_request,request_info->client_ip,request_info->client_port,"192.168.1.5",request_info->listen_port
                ,request_info->server_ip,request_info->server_port,request_len,response_len);
    write(request_info->access_log,log,strlen(log));
    int ret = flock(request_info->access_log,LOCK_UN);
    if(ret == -1)
    {
        printf("unlockerror\n");
        exit(0);
    }
    //printf("sum: %d , sumbuf: %s\n",sum,buf);
    

    close(server_fd);
    close(request_info->client_fd);
    request_info->close_time = time(NULL);
    request_info->conn_time = request_info->close_time - request_info->open_time;
    printf("sourceip: %s ; sourceport: %d\ndesip: %s ; desport: %d \n",request_info->client_ip,request_info->client_port,request_info->server_ip,request_info->server_port);
    printf("opentime: %d ; closetime: %d ; conntime: %d ;\n",request_info->open_time, request_info->close_time, request_info->conn_time);
    free(request_info);
    pthread_exit(NULL);
}
