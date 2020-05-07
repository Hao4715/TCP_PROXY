#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
//#include "thread_pool.h"


#define DEFALT_TIME 10           //10s检测一次
#define MIN_WAIT_TASK_NUM 10    //如果queue_size>MIN_WAIT_TASK_NUM添加新线程到线程池
#define DEFALT_THREAD_VARY 10   //每次创建和销毁的线程个数
#define true 1
#define false 0

typedef struct{
    void *(*function)(void *);   //函数指针，回调函数
    void *arg;                   //回调函数的参数
} threadpool_task_t;             //各子线程任务结构体

//描述线程池相关信息
struct threadpool_t {
    pthread_mutex_t lock;             //用于锁住本结构体
    pthread_mutex_t thread_counter;   //记录忙状态线程个数的锁
    pthread_cond_t queue_not_full;   //当任务队列满时，添加任务的线程阻塞，等待此条件变量
    pthread_cond_t queue_not_empty;  //当任务队列不为空时，通知等待任务的线程

    pthread_t *threads;               //存放线程池中每个线程的tid，数组
    pthread_t adjust_tid;             //存储管理线程的tid
    threadpool_task_t *task_queue;    //任务队列

    int min_thr_num;                  //线程池最小线程数
    int max_thr_num;                  //线程池最大线程数
    int live_thr_num;                 //当前存活线程个数
    int busy_thr_num;                 //忙状态线程个数
    int wait_exit_thr_num;            //要销毁的线程个数
    
    int queue_front;                  //task_queue队头下标
    int queue_rear;                   //task_queue队尾下标
    int queue_size;                   //task_queue队中实际任务数
    int queue_max_size;               //task_queue队列可容纳任务数上限

    int shutdown;                     //标志位，线程池使用状态，true或false
};

void *adjust_thread(void *threadpool);
void *threadpool_thread(void *threadpool);
int is_thread_alive(pthread_t tid);
int threadpool_free(struct threadpool_t *pool);
int threadpool_destroy(struct threadpool_t *pool);

struct threadpool_t *threadpool_create(int min_thr_num, int max_thr_num, int queue_max_size)
{
    int i;
    struct threadpool_t *pool = NULL;
    do{
        if((pool = (struct threadpool_t *)malloc(sizeof(struct threadpool_t))) == NULL){
            printf("malloc threadpool fail\n");
            break;
        }
        
        pool->min_thr_num = min_thr_num;
        pool->max_thr_num = max_thr_num;
        pool->busy_thr_num = 0;
        pool->live_thr_num = min_thr_num;              //活着线程数，初值为最小线程数
        pool->queue_size = 0;                          //有0个产品
        pool->queue_max_size = queue_max_size;
        pool->queue_front = 0;
        pool->queue_rear = 0;
        pool->shutdown = false;                        //不关闭线程池 

        //根据最大线程上限数，给工作线程数组开辟空间，并清零
        pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * max_thr_num);
        if(pool->threads == NULL){
            printf("malloc threads fail\n");
            break;
        }
        memset(pool->threads, 0, sizeof(pthread_t)*max_thr_num);

        //工作队列开辟空间
        pool->task_queue = (threadpool_task_t *)malloc(sizeof(threadpool_task_t) * queue_max_size);
        if(pool->task_queue == NULL){
            printf("malloc taskqueue fail\n");
            break;
        }
        memset(pool->task_queue, 0, sizeof(threadpool_task_t)*queue_max_size);

        //初始化互斥锁和条件变量
        if(pthread_mutex_init(&(pool->lock), NULL) !=0
            || pthread_mutex_init(&(pool->thread_counter), NULL) != 0
            || pthread_cond_init(&(pool->queue_not_empty), NULL) != 0
            || pthread_cond_init(&(pool->queue_not_full), NULL) != 0){
                printf("init the lock or cond fail\n");
                break;
        }

        //启动 min_thr_num 个 work thread
        for ( i = 0; i < min_thr_num; i++ ){
            //threadpool_thread是回调函数
            pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void *)pool);
            printf("start thread 0x%x...\n",(unsigned int)pool->threads[i]);
        }

        //管理线程
        pthread_create(&(pool->adjust_tid), NULL, adjust_thread, (void *)pool);
        return pool;
    } while(0);

    threadpool_free(pool);//上面初始化失败，释放pool空间
    return NULL;
}

//向线程池中 添加一个任务
int threadpool_add(struct threadpool_t *pool, void*(*function)(void *arg), void *arg){
    pthread_mutex_lock(&(pool->lock));

    //==为真 ， 队列已满， 调wait阻塞
    while((pool->queue_size == pool->queue_max_size) && (!pool->shutdown)){
        pthread_cond_wait(&(pool->queue_not_full), &(pool->lock));
    }
    if(pool->shutdown) {
        pthread_mutex_unlock(&(pool->lock));
    }

    //清空 工作线程 调用的回调函数 的参数 arg
    if(pool->task_queue[pool->queue_rear].arg != NULL){
        free(pool->task_queue[pool->queue_rear].arg);
        pool->task_queue[pool->queue_rear].arg = NULL;
    }

    //添加任务到队列里
    pool->task_queue[pool->queue_rear].function = function;
    pool->task_queue[pool->queue_rear].arg = arg;
    pool->queue_rear = (pool->queue_rear + 1) % pool->queue_max_size;
    pool->queue_size++;

    //添加任务后，队列不为空， 唤醒线程池中 等待处理任务的线程
    pthread_cond_signal(&(pool->queue_not_empty));
    pthread_mutex_unlock(&(pool->lock));

    return 0;
}

//各线程的工作者线程
void *threadpool_thread(void *threadpool){
    struct threadpool_t *pool = (struct threadpool_t *)threadpool;
    threadpool_task_t task;
    while (true){
        //刚创建的线程，等待任务队列有任务，否则阻塞等待任务队列里有任务后再唤醒接受任务
        pthread_mutex_lock(&(pool->lock));

        //queue_size==0 说明没有任务，调wait 阻塞在条件变量上，若有任务，跳过该while
        while((pool->queue_size == 0) && (!pool->shutdown)) {
            printf("thread 0x%x is waiting\n",(unsigned int)pthread_self());
            pthread_cond_wait(&(pool->queue_not_empty), &(pool->lock));

            //清除制定数目的空闲线程，如果要结束的线程数大于零，结束线程
            if(pool->wait_exit_thr_num > 0) {
                pool->wait_exit_thr_num--;

                //如果线程池里线程数大于最小值时可以结束当前线程
                if(pool->live_thr_num > pool->min_thr_num) {
                    printf("thread 0x%x is exiting\n", (unsigned int)pthread_self());
                    pool->live_thr_num--;
                    pthread_mutex_unlock(&(pool->lock));
                    pthread_exit(NULL);
                }
            }
        }

        //如果指定了true，要关闭线程池的每个线程，自行退出处理
        if(pool->shutdown){
            pthread_mutex_unlock(&(pool->lock));
            printf("thread 0x%x is exiting\n",(unsigned int)pthread_self());
            pthread_exit(NULL);
        }

        //从任务队列里获取任务，是一个出队操作
        task.function = pool->task_queue[pool->queue_front].function;
        task.arg = pool->task_queue[pool->queue_front].arg;

        pool->queue_front = (pool->queue_front + 1) % pool->queue_max_size;//出队，模拟循环队列
        pool->queue_size--;

        //通知可以有新任务添加进来
        pthread_cond_broadcast(&(pool->queue_not_full));

        //任务取出后，立即将 线程池锁 释放
        pthread_mutex_unlock(&(pool->lock));

        //执行任务
        printf("thread 0c%x starting working\n",(unsigned int)pthread_self());
        pthread_mutex_lock(&(pool->thread_counter));
        pool->busy_thr_num++;
        pthread_mutex_unlock(&(pool->thread_counter));
        (*(task.function))(task.arg);

        //任务处理结束
        printf("thread 0x%x end working\n",(unsigned int)pthread_self());
        pthread_mutex_lock(&(pool->thread_counter));
        pool->busy_thr_num--;
        pthread_mutex_unlock(&(pool->thread_counter));
    }

    pthread_exit(NULL);
    
}

//管理线程
void *adjust_thread(void *threadpool){
    int i;
    struct threadpool_t *pool = (struct threadpool_t *)threadpool;

    while (!pool->shutdown)
    {
        sleep(DEFALT_TIME);       //定时 对线程池管理

        pthread_mutex_lock(&(pool->lock));
        int queue_size = pool->queue_size;    //任务数
        int live_thr_num = pool->live_thr_num;//存活线程数
        pthread_mutex_unlock(&(pool->lock));

        pthread_mutex_lock(&(pool->thread_counter));
        int busy_thr_num = pool->busy_thr_num;  //繁忙线程数
        pthread_mutex_unlock(&(pool->thread_counter));

        //创建新线程 算法： 任务数大于最小线程池个数，且存活的线程数少于最大线程个数，如：30>=10 && 40<100
        if(queue_size >= MIN_WAIT_TASK_NUM && live_thr_num < pool->max_thr_num) {
            pthread_mutex_lock(&(pool->lock));
            int add = 0 ;

            //一次增加 DEFAULT——THREAD 个线程
            for( i = 0; i< pool->max_thr_num && add < DEFALT_THREAD_VARY && pool->live_thr_num < pool->max_thr_num; i++){
                if(pool->threads[i] == 0 || is_thread_alive(pool->threads[i])){
                    pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void *)pool);
                    add++;
                    pool->live_thr_num++;
                }
            }

            pthread_mutex_unlock(&(pool->lock));
        }

        //销毁多余的空闲线程 算法：忙线程*2 小于 存活的线程数 且 存活线程数 大于 最小线程数 时
        if((busy_thr_num * 2) < live_thr_num && live_thr_num > pool->min_thr_num) {

            //一次销毁DEFALT——THREAD个线程，随机DEFALT——THREAD个即可
            pthread_mutex_lock(&(pool->lock));
            pool->wait_exit_thr_num = DEFALT_THREAD_VARY;
            pthread_mutex_unlock(&(pool->lock));

            for( i = 0 ; i < DEFALT_THREAD_VARY; i++) {
                //通知空闲状态的线程，会自行中止
                pthread_cond_signal(&(pool->queue_not_empty));
            }
        }
    }

    return NULL;
}
//销毁线程
int threadpool_destroy(struct threadpool_t *pool) {
    int i;
    if (pool == NULL){
        return -1;
    }
    pool->shutdown = true;
    
    //先销毁管理线程、
    pthread_join(pool->adjust_tid, NULL);

    for(i = 0; i< pool->live_thr_num; i++) {
        //通知所有空闲线程
        pthread_cond_broadcast(&(pool->queue_not_empty));
    }

    for(i = 0; i < pool->live_thr_num ; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    threadpool_free(pool);

    return 0;
}

int threadpool_free(struct threadpool_t *pool) {
    if (pool == NULL ) {
        return -1;
    }

    if( pool->task_queue ) {
        free(pool->task_queue);
    }

    if( pool->threads ) {
        free(pool->threads);
        pthread_mutex_lock(&(pool->lock));
        pthread_mutex_destroy(&(pool->lock));
        pthread_mutex_lock(&(pool->thread_counter));
        pthread_mutex_destroy(&(pool->thread_counter));
        pthread_cond_destroy(&(pool->queue_not_empty));
        pthread_cond_destroy(&(pool->queue_not_full));
    }

    free(pool);
    pool = NULL;
     return 0;
}

int is_thread_alive(pthread_t tid) {
    int kill_rc = pthread_kill(tid, 0);
    if(kill_rc == ESRCH) {
        return false;
    }

    return true;
}

void *process(void *arg){
    printf("thread Ox%x working on task %d\n",(unsigned int)pthread_self(),*(int *)arg);
    sleep(1);
    printf("task %d is end\n",*(int *)arg);
    return NULL;
}

int main(){
    struct threadpool_t *thp = threadpool_create(3,100,100);//创建线程池，池中最小3个线程，最大100，队列最大100
    printf("pool inited\n");

    int num[20],i;
    for(i=0;i<20;i++){
        num[i] = i;
        printf("add task %d\n",i);
        threadpool_add(thp, process, (void *)&num[i]);
    }
    sleep(10);
    threadpool_destroy(thp);
    return 0;
}