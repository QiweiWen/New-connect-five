//
//  thread.h
//  Connect-five
//
//  Created by Qiwei Wen on 4/02/13.
//  Copyright (c) 2013 Qiwei Wen. All rights reserved.
//

#ifndef Connect_five_thread_h
#define Connect_five_thread_h
#include <pthread.h>
#include "List.h"

int taskNum ;
int tempNum ;

int workDone;

int crazyOutputSignal;

typedef struct _thread{
    pthread_t worker;
    int avail;
}thread;

typedef struct _task{
    int taskID;
    int result;
    
    int arg1;
    int arg2;
    int arg3;
    void* arg4;
    int arg6;
    
    int arg7;
    
    U64 arg8;
    
    int (*func)(int,int,int,void*,int,int,int,U64);
}task;

typedef struct _Queue{
    Node first;
}queue;

typedef queue* Queue;

pthread_mutex_t* mutex_queue;

Queue workQueue;
Queue original;

thread* workers;

void queue_add_task(void*);
task* queue_do_task(Queue);

void threads_initialise(void);
void* thread_idle(void*);

void end(void);

void printResult(Queue q);

void thread_end(void);

#endif
