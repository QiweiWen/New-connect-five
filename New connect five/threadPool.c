//
//  threadPool.c
//  Connect-five
//
//  How to Use:
//  1. create tasks and insert them into the work queue
//  2. after inputting all tasks, input an ending NULL
//  3. wait for the threads to finish working
//  4. look up the queue "original" for results
//  *call initialise() when using for the first time
//  *don't forget to free any memory allocated thanks to the pointer nature of most of the variables involved.
//  *the main thread can be added to the work force too by letting it fetch work the same way these spawned threads do. Beware of variable name clashing though. (tempNum, taskNum, workQueue, etc, make sure the function in which work fetching happens don'thave these as local variable names.)
//
//  Created by Qiwei Wen on 4/02/13.
//  Copyright (c) 2013 Qiwei Wen. All rights reserved.
//

#include <stdio.h>
#include "threadPool.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

//terminates when NULL is received
void queue_add_task(void* item){
    if (item != NULL){
        if (workDone) workDone = 0;
        tempNum++;
        if (workQueue == NULL){
            workQueue = malloc(sizeof(struct _Queue));
            workQueue->first = createList(item);
            original = malloc(sizeof(queue));
            original->first = workQueue->first;
        }else{
            insertItem(item, workQueue->first);
        }
    }else{
        taskNum = tempNum;
    }
}

task* queue_do_task(Queue q){
    task* fetch;
    if (q != NULL){
        fetch = (task*)key(q->first);
        q->first= nextItem(q->first);
        
    }else{
        return NULL;
    }
    return fetch;
}


void threads_initialise(void){
    taskNum = 0;
    tempNum = 0;
    
    crazyOutputSignal = 0;
    
    workQueue = NULL;
    original = NULL;
    workers = malloc(sizeof(thread)*NUM_THREADS);
    int i = 0;
    int* arg;
    mutex_queue = malloc(sizeof(pthread_mutex_t));
    mutex_eval_chart = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex_queue, NULL);
    pthread_mutex_init(mutex_eval_chart,NULL);
    while (i < NUM_THREADS){
        arg = malloc(sizeof(int));
        *arg = i + 1;
        
        pthread_create(&workers[i].worker, NULL, &thread_idle,arg);
        workers[i].avail = 1;
        i++;
    }
}

void* thread_idle(void* arg){
    int threadID = *((int*)arg); 
    while (1 == 1){
        while (taskNum == 0){
            //sleep some time to avoid 100% CPU core usage
             usleep(100);
        }
        pthread_mutex_lock(mutex_queue);
        task* fetch = queue_do_task(workQueue);
        pthread_mutex_unlock(mutex_queue);
        if (fetch != NULL){
            fetch->result = fetch->func(fetch->arg1,fetch->arg2,fetch->arg3,fetch->arg4,threadID,fetch->arg6,fetch->arg7,fetch->arg8);
                       
            taskNum--;
            tempNum--;
            if (taskNum == 0){
                workDone = 1;
            }
        }
    }
    return NULL;
}

void printResult(Queue q){
    int i = 0;
    Node current = q->first;
    while (current != NULL){
        i = ((task*)key(current))->result;
        printf("%d/",i);
        current = nextItem(current);
    }
    printf("\n");
}

void thread_end(void){
    int i =0;
    while (i <  NUM_THREADS){
        pthread_cancel(workers[i].worker);
        i++;
    }
    pthread_mutex_destroy(mutex_queue);
    pthread_mutex_destroy(mutex_eval_chart);
    free(workers);
}