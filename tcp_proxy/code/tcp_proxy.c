/*
* @brief     进程函数，逻辑主体
* @details   包含代理进程函数以及工作线程函数
*/
#include "../header/tcp_proxy.h"

// int test;
pthread_mutex_t test_mutex;
// pthread_cond_t test_cond = PTHREAD_COND_INITIALIZER;


//代理程序进程函数
void proxy_process(int listen_port, char *server_ip, int server_port,int access_log,struct statistics *statistics_info)
{

    pthread_mutex_init(&test_mutex,NULL);
    // pthread_cond_init(&test_cond,NULL);

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
        //printf("%d process waiting for %d connection.....\n",getpid(),num);
        int client_fd = accept(listen_fd,(struct sockaddr *)&client_addr, &client_addr_len);
        if(client_fd == -1)
            printf("accept error\n");
        //客户端请求信息初始化
        request_info->statistics_info = statistics_info;
        
        //互斥访问客户端每秒连接数以及总连接数
        pthread_mutex_lock(&(request_info->statistics_info->client_proxy_connections_mutex));
        statistics_info->client_proxy_CPS++;
        statistics_info->client_proxy_connections_all++;
        pthread_mutex_unlock(&(request_info->statistics_info->client_proxy_connections_mutex));
        
        //互斥访问正在处理连接数
        pthread_mutex_lock(&(request_info->statistics_info->client_proxy_connections_now_mutex));
        request_info->statistics_info->client_proxy_connections_now++;
        pthread_mutex_unlock(&(request_info->statistics_info->client_proxy_connections_now_mutex));

        request_info->open_time = time(NULL);
        request_info->client_fd = client_fd;

        strcpy(request_info->server_ip, server_ip);
        request_info->server_port = server_port;
        request_info->listen_port = listen_port;
        inet_ntop(AF_INET,&client_addr.sin_addr,request_info->client_ip, sizeof(request_info->client_ip));
        request_info->client_port = client_addr.sin_port;

        request_info->access_log = access_log;

        //printf("this is %d client ...\n",num);
        res = pthread_create(&th, NULL, handle_request, (void *)request_info);
        if(res != 0)
        {
            printf("thread error %s\n",strerror(res));
        }
        //pthread_detach(th);
    }
    close(listen_fd);
}


//线程处理函数
void *handle_request(void *arg)
{
    pthread_detach(pthread_self());


    char *log_format = "%s - - [ %s ] \"%s\" [%s:%d]--[%s:%d]--[%s:%d] request_length:%d  response_length:%d\n";
    char log[512];
    struct request_info *request_info = (struct request_info*)arg;
    int request_len=0,len,response_len=0;
    char buffer_request[1024],buffer_response[1024];
    int server_fd;
    struct sockaddr_in server_addr;
    
    request_len = recv(request_info->client_fd,buffer_request,sizeof(buffer_request),0);
    if(request_len == 0)
    {
        close(request_info->client_fd);
        pthread_mutex_lock(&(request_info->statistics_info->client_proxy_connections_now_mutex));
        request_info->statistics_info->client_proxy_connections_now--;
        pthread_mutex_unlock(&(request_info->statistics_info->client_proxy_connections_now_mutex));
        free(request_info);
        request_info = NULL;
        pthread_exit(NULL);
    }
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd == -1)
        printf("socket to server error\n");
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(request_info->server_port);
    server_addr.sin_addr.s_addr = inet_addr(request_info->server_ip);
    pthread_mutex_lock(&test_mutex);
    if( connect(server_fd, (struct sockaddr*)&server_addr,sizeof(server_addr)) < 0){
        printf("connect error\n");
    }
    pthread_mutex_unlock(&test_mutex);


    

    //互斥访问代理程序与服务器每秒建立连接数以及总连接数
    pthread_mutex_lock(&(request_info->statistics_info->proxy_server_connections_mutex));
    request_info->statistics_info->proxy_server_CPS++;
    request_info->statistics_info->proxy_server_connections_all++;
    pthread_mutex_unlock(&(request_info->statistics_info->proxy_server_connections_mutex));

    //互斥访问正在处理的代理程序与服务器连接数
    pthread_mutex_lock(&(request_info->statistics_info->proxy_server_connections_now_mutex));
    request_info->statistics_info->proxy_server_connections_now++;
    pthread_mutex_unlock(&(request_info->statistics_info->proxy_server_connections_now_mutex));

    
    
    //printf("read:%s", buffer_request);
    int test_send = send(server_fd, buffer_request, request_len, 0);
    //int test_send = send(server_fd, buffer_request, sizeof(buffer_request), 0);
    if(test_send <= 0)
        printf("send error\n");
    //互斥更新传输数据量
    pthread_mutex_lock(&(request_info->statistics_info->request_data_mutex));
    request_info->statistics_info->client_to_proxy_data += request_len;
    request_info->statistics_info->proxy_to_server_data += request_len;
    pthread_mutex_unlock(&(request_info->statistics_info->request_data_mutex));

    //pthread_mutex_lock(&test_mutex);
    //pthread_cond_signal(&test_cond);
    //pthread_mutex_lock(&test_mutex);
    //pthread_cond_wait(&test_cond,&test_mutex);
    //pthread_mutex_unlock(&test_mutex);
    // while(1)
    // {}
    while ((len = recv(server_fd, buffer_response, sizeof(buffer_response), 0)) > 0)
    {
        
        response_len += len;
        //send(request_info->client_fd, buffer_response, sizeof(response_len), 0);
        if (send(request_info->client_fd, buffer_response, len, 0) <= 0)
            printf("send to client error\n");
        //互斥更新传输数据量
        pthread_mutex_lock(&(request_info->statistics_info->response_data_mutex));
        request_info->statistics_info->proxy_to_client_data += len;
        request_info->statistics_info->server_to_proxy_data += len;
        pthread_mutex_unlock(&(request_info->statistics_info->response_data_mutex));
    }
  
    // pthread_mutex_lock(&test_mutex);
    // test++;
    // //if(test % 200 == 0)
    // printf("num:%d\n",test);
    // pthread_mutex_unlock(&test_mutex);

    sprintf(log,log_format,"192.168.1.11",ctime(&request_info->open_time),buffer_request,request_info->client_ip,request_info->client_port,"192.168.1.11",request_info->listen_port
                ,request_info->server_ip,request_info->server_port,request_len,response_len);
    
    if(flock(request_info->access_log,LOCK_EX) < 0)
    {
        printf("lockerrorrrrrrrr\n");
        exit(0);
    }
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
    //printf("sourceip: %s ; sourceport: %d\ndesip: %s ; desport: %d \n",request_info->client_ip,request_info->client_port,request_info->server_ip,request_info->server_port);
    //printf("opentime: %d ; closetime: %d ; conntime: %d ;\n",request_info->open_time, request_info->close_time, request_info->conn_time);
    
    //互斥统计完成连接数
    pthread_mutex_lock(&(request_info->statistics_info->connections_finished_mutex));
    request_info->statistics_info->client_proxy_connections_finished++;
    request_info->statistics_info->proxy_server_connections_finished++;
    pthread_mutex_unlock(&(request_info->statistics_info->connections_finished_mutex));

    pthread_mutex_lock(&(request_info->statistics_info->client_proxy_connections_now_mutex));
    request_info->statistics_info->client_proxy_connections_now--;
    pthread_mutex_unlock(&(request_info->statistics_info->client_proxy_connections_now_mutex));

    pthread_mutex_lock(&(request_info->statistics_info->proxy_server_connections_now_mutex));
    request_info->statistics_info->proxy_server_connections_now--;
    pthread_mutex_unlock(&(request_info->statistics_info->proxy_server_connections_now_mutex));

    free(request_info);
    request_info = NULL;
    pthread_exit(NULL);
}
