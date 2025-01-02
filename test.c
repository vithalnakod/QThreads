/*
 * file:        test.c
 * description: unit tests for custom qthread library
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "qthread.h"
 
/* Helper function to print completion message */
void test_passed(const char* test_name) {
    printf("%s passed successfully.\n", test_name);
}
 
/* Basic single thread test */
void* basic_thread(void* arg) {
    printf("Hello from basic thread!\n");
    return NULL;
}
 
void test_single_thread(void) {
    printf("Running single thread test...\n");
    qthread_t th = qthread_create(basic_thread, NULL);
    qthread_join(th);
    test_passed("test_single_thread");
}
 
/* Test with two threads */
void* thread_print(void* arg) {
    int num = *((int*)arg);
    printf("Hello from thread %d\n", num);
    return NULL;
}
 
void test_two_threads(void) {
    printf("Running two threads test...\n");
    int arg1 = 1, arg2 = 2;
    qthread_t th1 = qthread_create(thread_print, &arg1);
    qthread_t th2 = qthread_create(thread_print, &arg2);
    qthread_join(th1);
    qthread_join(th2);
    test_passed("test_two_threads");
}
 
/* Test with three threads */
void test_three_threads(void) {
    printf("Running three threads test...\n");
    int arg1 = 1, arg2 = 2, arg3 = 3;
    qthread_t th1 = qthread_create(thread_print, &arg1);
    qthread_t th2 = qthread_create(thread_print, &arg2);
    qthread_t th3 = qthread_create(thread_print, &arg3);
    qthread_join(th1);
    qthread_join(th2);
    qthread_join(th3);
    test_passed("test_three_threads");
}
 
/* Test for join after exit */
void* exit_thread(void* arg) {
    printf("Thread exiting...\n");
    return NULL;
}
 
void test_join_after_exit(void) {
    printf("Running join after exit test...\n");
    qthread_t th = qthread_create(exit_thread, NULL);
    qthread_join(th);
    test_passed("test_join_after_exit");
}
 
/* Yield test: Threads yield control voluntarily */
void* yield_test_thread(void* arg) {
    printf("Thread yielding...\n");
    qthread_yield();
    printf("Thread resumed after yield.\n");
    return NULL;
}
 
void test_yield(void) {
    printf("Running yield test...\n");
    qthread_t th = qthread_create(yield_test_thread, NULL);
    qthread_join(th);
    test_passed("test_yield");
}
 
/* Mutex lock/unlock test */
void* mutex_test_thread(void* arg) {
    qthread_mutex_t* mutex = (qthread_mutex_t*)arg;
    qthread_mutex_lock(mutex);
    printf("Thread acquired mutex lock.\n");
    qthread_mutex_unlock(mutex);
    printf("Thread released mutex lock.\n");
    return NULL;
}
 
void test_mutex(void) {
    printf("Running mutex test...\n");
    qthread_mutex_t* mutex = qthread_mutex_create();
    qthread_t th1 = qthread_create(mutex_test_thread, mutex);
    qthread_t th2 = qthread_create(mutex_test_thread, mutex);
    qthread_join(th1);
    qthread_join(th2);
    qthread_mutex_destroy(mutex);
    test_passed("test_mutex");
}
 
/* Condition variable wait/signal test */
void* cond_wait_thread(void* arg) {
    qthread_cond_t* cond = ((void**)arg)[0];
    qthread_mutex_t* mutex = ((void**)arg)[1];
 
    qthread_mutex_lock(mutex);
    printf("Thread waiting on condition variable...\n");
    qthread_cond_wait(cond, mutex);
    printf("Thread resumed after condition signal.\n");
    qthread_mutex_unlock(mutex);
    return NULL;
}
 
void test_cond_wait_signal(void) {
    printf("Running condition wait/signal test...\n");
    qthread_cond_t* cond = qthread_cond_create();
    qthread_mutex_t* mutex = qthread_mutex_create();
    void* args[] = {cond, mutex};
 
    qthread_t th = qthread_create(cond_wait_thread, args);
    qthread_usleep(100000);  // Sleep for a bit to ensure wait
    qthread_mutex_lock(mutex);
    printf("Signaling condition variable...\n");
    qthread_cond_signal(cond);
    qthread_mutex_unlock(mutex);
 
    qthread_join(th);
    qthread_cond_destroy(cond);
    qthread_mutex_destroy(mutex);
    test_passed("test_cond_wait_signal");
}
 
/* Condition variable broadcast test */
void test_cond_broadcast(void) {
    printf("Running condition broadcast test...\n");
    qthread_cond_t* cond = qthread_cond_create();
    qthread_mutex_t* mutex = qthread_mutex_create();
    void* args[] = {cond, mutex};
 
    qthread_t th1 = qthread_create(cond_wait_thread, args);
    qthread_t th2 = qthread_create(cond_wait_thread, args);
    qthread_usleep(100000);  // Ensure threads are waiting
 
    qthread_mutex_lock(mutex);
    printf("Broadcasting condition variable...\n");
    qthread_cond_broadcast(cond);
    qthread_mutex_unlock(mutex);
 
    qthread_join(th1);
    qthread_join(th2);
    qthread_cond_destroy(cond);
    qthread_mutex_destroy(mutex);
    test_passed("test_cond_broadcast");
}
 
/* Sleep test */
void* sleep_thread(void* arg) {
    printf("Thread sleeping for 1 second...\n");
    qthread_usleep(1000000);  // Sleep for 1 second
    printf("Thread woke up after sleep.\n");
    return NULL;
}
 
void test_sleep(void) {
    printf("Running sleep test...\n");
    qthread_t th = qthread_create(sleep_thread, NULL);
    qthread_join(th);
    test_passed("test_sleep");
}
 
/* Main function to run all tests */
int main(void) {
    qthread_init();
 
    test_single_thread();
    test_two_threads();
    test_three_threads();
    test_join_after_exit();
    test_yield();
    test_mutex();
    test_cond_wait_signal();
    test_cond_broadcast();
    test_sleep();
 
    printf("All tests completed successfully.\n");
    return 0;
}