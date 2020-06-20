#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

struct th
{
    /* data */
    int num;
    pthread_mutex_t th_mutex;
    pthread_mutexattr_t th_mutex_attr;
};

int i=0;

void *add(void * arg)
{
    struct th *th_info = (struct th*)arg;
    pthread_mutex_lock(&(th_info->th_mutex));
    i++;
    th_info->num++;
    printf("%d: %d\n",i,th_info->num);
    pthread_mutex_unlock(&(th_info->th_mutex));
    pthread_exit(NULL);
}

void process(struct th * th_info)
{
    int i;
    pthread_t th[10];
    for(i=0;i<10;i++)
    {
        //printf("pid:%d==== %d\n",getpid(),i+1);
        int res = pthread_create(&th[i],NULL,add,(void *)th_info);
        //pthread_detach(th);
        /*pthread_mutex_lock(&(th_info->th_mutex));
        th_info->num++;
        printf("pid: %d %d: %d\n",getpid(), i+1, th_info->num);
        pthread_mutex_unlock(&(th_info->th_mutex));*/
    }
    for(i = 9;i >=0;i--)
    {
        if(th[i] != -1)
        {
            pthread_join(th[i],NULL);
        }
    }
    printf("jieshu\n");
    exit(1);
}

int main()
{
    int i;
    pid_t p_id;
    //struct th *th_info = (struct th*)malloc(sizeof(struct th));
    int fd = open("/dev/zero",O_RDWR);
    struct th *th_info = mmap(NULL,sizeof(struct th),PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
    th_info->num = 0;

    pthread_mutexattr_init(&(th_info->th_mutex_attr));
    pthread_mutexattr_setpshared(&(th_info->th_mutex_attr),PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(th_info->th_mutex),&(th_info->th_mutex_attr));
    //pthread_mutex_init(&(th_info->th_mutex),NULL);
    for(i=0;i<3;i++)
    {
        if( (p_id = fork()) ==0 )
        {
            //printf("pid:%d\n",p_id);
            process(th_info);
        }
    }
    int stats,res=0;
    i=0;
    while(1)
    {
        if((res = wait(&stats)) != -1) i++;
        if(i >= 3) break;
    }
    printf("dfsd\n");
    return 0;
}
