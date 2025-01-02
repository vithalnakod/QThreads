/*
 * file:        qthread.c
 * description: assignment - simple emulation of POSIX threads
 * class:       CS 5600, Fall 2019
 */

/* a bunch of includes which will be useful */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/time.h>
#include <errno.h>
#include <stdbool.h>
#include "qthread.h"

/* prototypes for stack.c and switch.s
 * see source files for additional details
 */
extern void switch_thread(void **location_for_old_sp, void *new_value);
extern void *setup_stack(void *_stack, size_t len, void *func, void *arg1, void *arg2);

typedef struct qthread {
    struct qthread* next;
    void* saved_sp;
    long int wake_time;
    void* return_val;
    int exit_flag;
    qthread_t wait_thread;
    void* stack;
} qthread;  


/* You'll probably want to define a thread queue structure, and
 * functions to append and remove threads. (Note that you only need to
 * remove the oldest item from the head, makes removal a lot easier)
 */
typedef struct qthread_queue {
    qthread* front;
    qthread* back;
} qthread_queue;

/* Mutex and cond structures - @allocate them in qthread_mutex_create / 
 * qthread_cond_create and free them in @the corresponding _destroy functions.
 */
struct qthread_mutex {
    /* your code here */;
    int locked;
    qthread_queue mutex_waiting_queue;
};

struct qthread_cond {
    /* your code here */;
    qthread_queue cond_waiting_queue;
};

static qthread *current;
static qthread_queue active_queue;
static qthread *threads[32];
static qthread *threads_in_sleep[32];

/* Helper function for POSIX replacement API - you'll need to tell
 * time in order to implement qthread_usleep. 
 * WARNING - store return value in 'long' (64 bits), not 'int' (32 bits)
 */
static long int get_usecs()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec*1000000 + tv.tv_usec;
}

bool empty(qthread_queue * queue){
    return queue->front==NULL;
}

void append(qthread_queue * queue, qthread* th){
    if(empty(queue)){
        queue->front=th;
        queue->back=th;
        th->next=NULL;
    }
    else{
        queue->back->next=th;
        queue->back=th;
        th->next=NULL;
    }
}


qthread* pop(qthread_queue * queue){
    if(empty(queue)) {
        return NULL;
    }
    else if(queue->front==queue->back){
        qthread * frontThread = queue->front;
        queue->front=NULL;
        queue->back=NULL;
        return frontThread;
    }
    else{
        qthread * frontThread = queue->front;
        queue->front = frontThread->next;
        return frontThread;
    }
    
}

void schedule(void *save_location) {
    long current_time = get_usecs();

    for (int i = 0;i<32;i++) {
        if ((threads_in_sleep[i]) && current_time >= threads_in_sleep[i]->wake_time) {
            append(&active_queue, threads_in_sleep[i]);
            threads_in_sleep[i]=NULL;
        }
    }

    qthread *next = pop(&active_queue);
    if (!next) {
        long min_wake_time = LONG_MAX;
        for (int i = 0;i<32;i++){
            if (threads_in_sleep[i] && threads_in_sleep[i]->wake_time < min_wake_time) {
                min_wake_time = threads_in_sleep[i]->wake_time;
            }
        }
        if (min_wake_time != LONG_MAX) {
            long time_to_sleep = min_wake_time - current_time;
            if (time_to_sleep > 0) {
                usleep(time_to_sleep);
            }
        }
        return schedule(save_location);
    }
    if (next != current) {
        current = next;
        switch_thread(save_location, next->saved_sp);
    }
}

qthread_t create_2arg_thread(f_2arg_t f, void *arg1, void *arg2)
{
  /* do all your thread creation stuff */
    qthread* new_thread = malloc(sizeof(qthread));
    new_thread->exit_flag = 0;
    new_thread->wait_thread = NULL;
    new_thread->wake_time = 0;
    new_thread->stack = malloc(65536);
    new_thread->saved_sp = setup_stack(new_thread->stack, 65536, f, arg1, arg2);
    append(&active_queue, new_thread);

    for (int i = 0; i < 32; i++) {
        if (threads[i] == NULL) {
            threads[i] = new_thread;
            break;
        }
    }

    return new_thread;
}

void wrapper(void *arg1, void *arg2)
{
    f_1arg_t f = arg1;
    void *tmp = f(arg2);
    qthread_exit(tmp);
}

/* qthread_init - set up a thread structure for the main (OS-provided) thread
 */
void qthread_init(void)
{
    current = malloc(sizeof(qthread));
    current->next = NULL;
    current->saved_sp = NULL;
    current->wait_thread = NULL;
    current->exit_flag = 0;
    current->wake_time = 0;
    current->return_val = NULL;
    current->stack = NULL;
    
    threads[0] = current;
    for (int i = 1; i < 32; i++) {
        threads[i] = NULL;
    }
}

/* qthread_create - see hints @for how to implement it, especially the
 * reference to a "wrapper" function
 */
qthread_t qthread_create(f_1arg_t f, void *arg1)
{
    return create_2arg_thread(wrapper, f, arg1);
}

/* qthread_yield - yield to the next @runnable thread.
 */
void qthread_yield(void)
{
    /* your code here */
    append(&active_queue, current);
    schedule(&current->saved_sp);
}

/* qthread_exit, qthread_join - exit argument is returned by
 * qthread_join. Note that join blocks if the thread hasn't exited
 * yet, and is allowed to crash @if the thread doesn't exist.
 */
void qthread_exit(void *val) {
    /* your code here */
    current->return_val = val;
    current->exit_flag = 1;
    if (current->wait_thread) {
        append(&active_queue, current->wait_thread);
    }
    schedule(&current->saved_sp);
}

void *qthread_join(qthread_t thread) {
    /* your code here */
    while (!thread->exit_flag) {
        if (thread->wait_thread != NULL) {
            exit(1);
        }
        thread->wait_thread = current;
        schedule(&current->saved_sp);
    }
    void *ret_val = thread->return_val;
    free(thread->stack);
    free(thread);
    return ret_val;
}

/* Mutex functions
 */
qthread_mutex_t *qthread_mutex_create(void)
{
    qthread_mutex_t *mutex = malloc(sizeof(qthread_mutex_t));
    mutex->locked = 0;
    mutex->mutex_waiting_queue.front = mutex->mutex_waiting_queue.back = NULL;
    return mutex;
}

void qthread_mutex_destroy(qthread_mutex_t *mutex)
{
    free(mutex);
}

void qthread_mutex_lock(qthread_mutex_t *mutex)
{
    if (!mutex->locked) {
        mutex->locked = 1;
    } else {
        append(&mutex->mutex_waiting_queue, current);
        schedule(&current->saved_sp);
    }
}

void qthread_mutex_unlock(qthread_mutex_t *mutex)
{
    if (!empty(&mutex->mutex_waiting_queue)) {
        struct qthread *next = pop(&mutex->mutex_waiting_queue);
        append(&active_queue, next);
    } else {
        mutex->locked = 0;
    }
}

/* Condition variable functions
 */
qthread_cond_t *qthread_cond_create(void)
{
    qthread_cond_t *cond = malloc(sizeof(qthread_cond_t));
    cond->cond_waiting_queue.front = cond->cond_waiting_queue.back = NULL;
    return cond;
}

void qthread_cond_destroy(qthread_cond_t *cond)
{
    free(cond);
}


void qthread_cond_wait(qthread_cond_t *cond, qthread_mutex_t *mutex)
{
    append(&cond->cond_waiting_queue, current);
    qthread_mutex_unlock(mutex);
    schedule(&current->saved_sp);
    qthread_mutex_lock(mutex);
    // struct qthread *next_thread = pop(&active_queue);
    // if (next_thread) {
    //     struct qthread *prev = current;
    //     current = next_thread;
    //     switch_thread(&prev->saved_sp, current->saved_sp);
    // }
    // qthread_mutex_lock(mutex);
}

void qthread_cond_signal(qthread_cond_t *cond)
{
    struct qthread *next = pop(&cond->cond_waiting_queue);
    if (next) {
        append(&active_queue, next);
    }
    
}
void qthread_cond_broadcast(qthread_cond_t *cond)
{
    while (!empty(&cond->cond_waiting_queue)) {
        struct qthread *next = pop(&cond->cond_waiting_queue);
        append(&active_queue, next);
        
    }
}


void qthread_usleep(long usecs) {
    long current_time = get_usecs() + usecs;
    for (int i =0;i<32;i++){
        if (threads_in_sleep[i] == NULL) {
            threads_in_sleep[i] = current;
            threads_in_sleep[i]->wake_time = current_time;
            break;
        }
    }
    schedule(&current->saved_sp);
}