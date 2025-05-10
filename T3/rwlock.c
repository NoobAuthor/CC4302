#define _XOPEN_SOURCE 500
#include <pthread.h>
#include <stdlib.h>
#include "rwlock.h"
#include "pss.h"

struct rwlock {
    pthread_mutex_t mutex;         
    pthread_cond_t read_cond;      
    pthread_cond_t write_cond;     
    
    int active_readers;            
    int active_writer;             
    
    Queue *waiting_readers;        
    Queue *waiting_writers;        
};

typedef struct {
    pthread_cond_t *cond;          
    int signaled;                  
} ThreadInfo;

RWLock *makeRWLock() {
    RWLock *rwl = (RWLock *)malloc(sizeof(RWLock));
    if (rwl == NULL) {
        return NULL;
    }
    
    pthread_mutex_init(&rwl->mutex, NULL);
    pthread_cond_init(&rwl->read_cond, NULL);
    pthread_cond_init(&rwl->write_cond, NULL);
    
    rwl->active_readers = 0;
    rwl->active_writer = 0;
    
    rwl->waiting_readers = makeQueue();
    rwl->waiting_writers = makeQueue();
    
    return rwl;
}

void destroyRWLock(RWLock *rwl) {
    if (rwl == NULL) {
        return;
    }
    
    while (!emptyQueue(rwl->waiting_readers)) {
        ThreadInfo *info = (ThreadInfo *)get(rwl->waiting_readers);
        pthread_cond_destroy(info->cond);
        free(info->cond);
        free(info);
    }
    
    while (!emptyQueue(rwl->waiting_writers)) {
        ThreadInfo *info = (ThreadInfo *)get(rwl->waiting_writers);
        pthread_cond_destroy(info->cond);
        free(info->cond);
        free(info);
    }
    
    destroyQueue(rwl->waiting_readers);
    destroyQueue(rwl->waiting_writers);
    
    pthread_cond_destroy(&rwl->read_cond);
    pthread_cond_destroy(&rwl->write_cond);
    pthread_mutex_destroy(&rwl->mutex);
    
    free(rwl);
}

void enterRead(RWLock *rwl) {
    pthread_mutex_lock(&rwl->mutex);
    
    if (rwl->active_writer == 0 && queueLength(rwl->waiting_writers) == 0) {
        rwl->active_readers++;
        pthread_mutex_unlock(&rwl->mutex);
        return;
    }
    
    ThreadInfo *info = (ThreadInfo *)malloc(sizeof(ThreadInfo));
    info->cond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    pthread_cond_init(info->cond, NULL);
    info->signaled = 0;
    
    put(rwl->waiting_readers, info);
    
    while (!info->signaled) {
        pthread_cond_wait(info->cond, &rwl->mutex);
    }
    
    pthread_cond_destroy(info->cond);
    free(info->cond);
    free(info);
    
    pthread_mutex_unlock(&rwl->mutex);
}

void exitRead(RWLock *rwl) {
    pthread_mutex_lock(&rwl->mutex);
    
    rwl->active_readers--;
    
    if (rwl->active_readers == 0 && !emptyQueue(rwl->waiting_writers)) {
        ThreadInfo *writer_info = (ThreadInfo *)get(rwl->waiting_writers);
        rwl->active_writer = 1;
        writer_info->signaled = 1;
        pthread_cond_signal(writer_info->cond);
    }
    
    pthread_mutex_unlock(&rwl->mutex);
}

void enterWrite(RWLock *rwl) {
    pthread_mutex_lock(&rwl->mutex);
    
    if (rwl->active_readers == 0 && rwl->active_writer == 0) {
        rwl->active_writer = 1;
        pthread_mutex_unlock(&rwl->mutex);
        return;
    }
    
    ThreadInfo *info = (ThreadInfo *)malloc(sizeof(ThreadInfo));
    info->cond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
    pthread_cond_init(info->cond, NULL);
    info->signaled = 0;
    
    put(rwl->waiting_writers, info);
    
    while (!info->signaled) {
        pthread_cond_wait(info->cond, &rwl->mutex);
    }
    
    pthread_cond_destroy(info->cond);
    free(info->cond);
    free(info);
    
    pthread_mutex_unlock(&rwl->mutex);
}

void exitWrite(RWLock *rwl) {
    pthread_mutex_lock(&rwl->mutex);
    
    rwl->active_writer = 0;
    
    if (!emptyQueue(rwl->waiting_readers)) {
        while (!emptyQueue(rwl->waiting_readers)) {
            ThreadInfo *reader_info = (ThreadInfo *)get(rwl->waiting_readers);
            rwl->active_readers++;
            reader_info->signaled = 1;
            pthread_cond_signal(reader_info->cond);
        }
    } 
    else if (!emptyQueue(rwl->waiting_writers)) {
        ThreadInfo *writer_info = (ThreadInfo *)get(rwl->waiting_writers);
        rwl->active_writer = 1;
        writer_info->signaled = 1;
        pthread_cond_signal(writer_info->cond);
    }
    
    pthread_mutex_unlock(&rwl->mutex);
}