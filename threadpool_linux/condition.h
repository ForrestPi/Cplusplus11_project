#ifndef CONDITION_H
#define CONDITION_H

#include <pthread.h>

typedef struct condition{
    pthread_mutex_t pmutex;
    pthread_cond_t pcond;
}conditon_t;

int condition_init(conditon_t *cond);
int condition_lock(conditon_t *cond);
int condition_unlock(conditon_t *cond);
int condition_wait(conditon_t *cond);
int condition_timedwait(conditon_t *cond,const struct timespec* abstime);
int condition_signal(conditon_t* cond);
int condition_broadcast(conditon_t *cond);
int condition_destroy(conditon_t *cond);

#endif // CONDITION_H
