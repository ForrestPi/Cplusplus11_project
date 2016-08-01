
#include "threadpool.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

void *thread_routine(void *arg){

    struct timespec abstime;
    int timeout;
    printf("thread 0x%x is starting\n",(int)pthread_self());
    threadpool_t *pool=(threadpool_t*)arg;
    while(1){
        timeout=0;
        conditon_lock(&pool->ready);
        //等待有任务到来或线程池销毁通知
        while(pool->first==NULL && !pool->quit){
            printf("thread 0x%x is waiting\n",(int)pthread_self());
            clock_gettime(CLOCK_REALTIME,&abstime);
            abstime.tv_sec+=2;
            int status=condition_timedwait(&pool->ready,&abstime);
            if(status==ETIMEDOUT){
                printf("thread 0x%x is wait timed out\n",(int)pthread_self());
                timeout=1;
                break;
            }
        }
        //等待到条件，处于工作状态
        pool->idle--;
        if(pool->first!=NULL){
            //从队头取出任务
            task_t *t=pool->first;
            pool->first=t->next;
            //执行任务需要一定时间，所以要解锁，以便生产者线程添加新任务
            condition_unlock(&pool->ready);
            t->run(t->arg);
            free(t);
            condition_lock(&pool->ready);
        }
        //如果等待到线程池销毁通知
        if(pool->quit && pool->first==NULL){
            pool->counter--;
            condition_unlock(&pool->ready);
            break;
        }

        if(timeout && pool->first==NULL){
            pool->counter--;
            condition_unlock(&pool->ready);
            break;
        }
        condition_unlock(&pool->ready);
    }
}

void threadpool_init(threadpool_t *pool, int threads){
    conditon_init(&pool->ready);
    pool->first=NULL;
    pool->last=NULL;
    pool->counter=0;
    pool->idle=0;
    pool->max_threads=threads;
    pool->quit=0;
}

//往线程池中添加任务
void threadpool_add_task(threadpool_t *pool, void *(*run)(void *), void *arg){
    //生成新任务
    task_t *newtask=(task_t *)malloc(sizeof(task_t));
    newtask->run=run;
    newtask->arg=arg;
    newtask->next=NULL;

    condition_lock(&pool->ready);
    //将任务添加到队列
    if(pool->first==NULL){
        pool->first=newtask;
    }
    else{
        pool->last->next=newtask;
    }
    pool->last=newtask;

    //如果有等待线程，则唤醒一个
    if(pool->idle>0){
        conditon_signal(&pool->ready);
    }
    else if(pool->counter<pool->max_threads){
        //没有等待线程，并且当前线程数不超过最大线程数，则创建一个新线程
        pthread_t tid;
        pthread_create(&tid,NULL,thread_routine,pool);
        pool->counter++;
    }
    condition_unlock(&pool->ready);
}

void threadpool_destroy(threadpool_t *pool){
    condition_lock(&pool->ready);
    if(pool->quit){
        condition_unlock(&pool->ready);
        return;
    }
    condition_lock(&pool->ready);
    pool->quit=1;
    if(pool->counter>0){
        if(pool->idle>0){
            condition_broadcast(&pool->ready);
        }
        //处于执行任务状态中的线程，不会收到广播
        //线程池需要等待执行任务中线程全部退出
    }
    condition_unlock(&pool->ready);
    condition_destroy(&pool->ready);
}
