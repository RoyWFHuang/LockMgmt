#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "lock_mang.h"
#include "macro.h"
#include "errorno.h"
#include "list.h"

pthread_barrier_t barr_lock;



tLockTable *_g_lock = NULL;
char randtable[] = \
"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

void *block_lock_thread_func(void *arg)
{
    int thread_id = (int*) arg;
    LKM_DEBUG_PRINT("thread_id = %d is start wait barrier\n",
        thread_id);
    pthread_barrier_wait(&barr_lock);
    block_lock(_g_lock, "/roy/MySync/aaa/bbb/", NULL);
    //sleep(1);
    //release_lock(_g_lock, "/roy/MySync/aaa/bbb/", NULL);
    return NULL;
}
long long int _g_lock_fail_time = 0;

void *try_lock_thread_func(void *arg)
{
    //long long int thread_id = (long long int*) arg;
    srand(time(NULL));
    sleep(1);
    int dir_lv = (rand()%2) + 1;
    int rand_table_len = strlen(randtable);

    char lock_path[32];
    memset(lock_path, 0, sizeof(char[32]));
    strcpy(lock_path, "/roy/MySync/");

    for(int i =0; i< dir_lv; i++)
    {
        int len = 1;
        char *dir_lv = calloc(1, sizeof(char)*(len +1));
        for(int j = 0; j < len;j++)
        {
            dir_lv[j] = randtable[(rand()%rand_table_len)];
        }
        strcat(lock_path, dir_lv);
        strcat(lock_path, "/");
    }
    LKM_DEBUG_PRINT("lock_path = %s\n", lock_path);

    pthread_barrier_wait(&barr_lock);
    LKM_DEBUG_PRINT("start lock_path = %s\n", lock_path);
    int ret = try_lock(_g_lock, lock_path, NULL);
    if(ERROR_CODE_SUCCESS != ret)
        __sync_fetch_and_add(&_g_lock_fail_time, 1);

    //sleep(1);
    //release_lock(_g_lock, "/roy/MySync/aaa/bbb/", NULL);
    return NULL;
}

void *try_lock_thread_fixed_path(void *arg)
{
    srand(time(NULL));
    sleep(1);
    int dir_lv = 2;
    int rand_table_len = strlen(randtable);

    char lock_path[32];
    memset(lock_path, 0, sizeof(char[32]));
    strcpy(lock_path, "/roy/MySync/");

    for(int i =0; i< dir_lv; i++) {
        int len = 1;

        char *dir_lv = calloc(1, sizeof(char)*(len +1));
        for(int j = 0; j < len;j++) {
            dir_lv[j] = randtable[(rand()%rand_table_len)];
        }
        strcat(lock_path, dir_lv);
        strcat(lock_path, "/");
    }
    LKM_DEBUG_PRINT("lock_path = %s\n", lock_path);

    pthread_barrier_wait(&barr_lock);
    LKM_DEBUG_PRINT("start lock_path = %s\n", lock_path);
    int ret = try_lock(_g_lock, lock_path, NULL);
    if(ERROR_CODE_SUCCESS != ret)
        __sync_fetch_and_add(&_g_lock_fail_time, 1);

    //sleep(1);
    //release_lock(_g_lock, "/roy/MySync/aaa/bbb/", NULL);
    return NULL;
}

void *try_lock_thread_the_same_path_func1(void *arg)
{
    char lock_path[32];
    memset(lock_path, 0, sizeof(char[32]));
    strcpy(lock_path, "/roy/MySync/aaa/bbb/");
    LKM_DEBUG_PRINT("lock_path = %s\n", lock_path);
    int ret = try_lock(_g_lock, lock_path, NULL);
    LKM_DEBUG_PRINT("lock_path = %s\n", lock_path);
    if(ERROR_CODE_SUCCESS != ret)
        __sync_fetch_and_add(&_g_lock_fail_time, 1);
    //print_lock_table(_g_lock);
    pthread_barrier_wait(&barr_lock);
    release_lock(_g_lock, lock_path, NULL);
    return NULL;
}

void *try_lock_thread_the_same_path_func2(void *arg)
{
    char lock_path[32];
    memset(lock_path, 0, sizeof(char[32]));
    strcpy(lock_path, "/roy/MySync/aaa/ccc/");
    LKM_DEBUG_PRINT("lock_path = %s\n", lock_path);
    int ret = try_lock(_g_lock, lock_path, NULL);
    LKM_DEBUG_PRINT("lock_path = %s\n", lock_path);
    if(ERROR_CODE_SUCCESS != ret)
        __sync_fetch_and_add(&_g_lock_fail_time, 1);
    //print_lock_table(_g_lock);
    pthread_barrier_wait(&barr_lock);
    release_lock(_g_lock, lock_path, NULL);
    return NULL;
}

void *try_lock_thread_the_same_path_func_with_no_release(void *arg)
{
    char lock_path[32];
    memset(lock_path, 0, sizeof(char[32]));
    strcpy(lock_path, "/roy/MySync/aaa/bbb/");
    LKM_DEBUG_PRINT("lock_path = %s\n", lock_path);
    pthread_barrier_wait(&barr_lock);
    int ret = try_lock(_g_lock, lock_path, NULL);
    if(ERROR_CODE_SUCCESS != ret)
        __sync_fetch_and_add(&_g_lock_fail_time, 1);
    //release_lock(_g_lock, lock_path, NULL);
    return NULL;
}


void *only_release_lock(void *arg)
{
    char lock_path[32];
    memset(lock_path, 0, sizeof(char[32]));
    strcpy(lock_path, "/roy/MySync/aaa/bbb/");
    LKM_DEBUG_PRINT("lock_path = %s\n", lock_path);
    release_lock(_g_lock, lock_path, NULL);
    return NULL;
}

int main()
{
    int ret = ERROR_CODE_SUCCESS;
    //_g_lock = initial_lock_table();

#if 0
    LKM_DEBUG_PRINT("\n");
    block_lock(_g_lock, "/roy/MySync/aaa/bbb/", NULL);
    LKM_DEBUG_PRINT("\n");
    if( ERROR_CODE_SUCCESS !=
        (ret = try_lock(_g_lock, "/roy/MySync/aaa/bbb/", NULL)) )
    {
        LKM_DEBUG_PRINT("try lock fail(%x)\n", ret);
    }
    print_lock_table(_g_lock);
    release_lock(_g_lock, "/roy/MySync/aaa/bbb/", NULL);
    print_lock_table(_g_lock);
#endif

#define LOCK_THREAD 3


    pthread_barrier_init(&barr_lock, NULL, (unsigned) LOCK_THREAD);
/*
    for (int thread = 0; thread < LOCK_THREAD/2; thread++) {
        pthread_create(
            &lock_thread[thread],
            NULL, block_lock_thread_func,
            (void*)thread);
    }
*/
#ifdef RAND_LV
    LKM_DEBUG_PRINT("\n");
    pthread_t lock_thread[LOCK_THREAD];
    for (long long int thread = 0; thread < LOCK_THREAD; thread++)
    {
    //LKM_DEBUG_PRINT("thread = %lld\n", thread);
        pthread_create(
            &lock_thread[thread],
            NULL, try_lock_thread_func,
            NULL);
    }
    LKM_DEBUG_PRINT("\n");
    for (long long int thread = 0; thread < LOCK_THREAD; thread++)
        pthread_join(lock_thread[thread], NULL);
#endif

#ifdef FIX_LV
    pthread_t lock_thread2[LOCK_THREAD];
    for (long long int thread = 0; thread < LOCK_THREAD; thread++)
    {
    //LKM_DEBUG_PRINT("thread = %lld\n", thread);
        pthread_create(
            &lock_thread2[thread],
            NULL, try_lock_thread_fixed_path,
            (void*)thread);
    }
    for (long long int thread = 0; thread < LOCK_THREAD; thread++)
        pthread_join(lock_thread2[thread], NULL);
#endif

    pthread_t lock_thread3;
    pthread_t lock_thread3_1;
    pthread_t lock_thread3_2;
    //char *test = calloc(0, sizeof(char[10]));
    pthread_create(
        &lock_thread3,
        NULL, try_lock_thread_the_same_path_func1,
        NULL);
    pthread_create(
        &lock_thread3_1,
        NULL, try_lock_thread_the_same_path_func2,
        NULL);
    pthread_create(
        &lock_thread3_2,
        NULL, try_lock_thread_the_same_path_func_with_no_release,
        NULL);

    pthread_join(lock_thread3, NULL);
    pthread_join(lock_thread3_1, NULL);
    pthread_join(lock_thread3_2, NULL);

    print_lock_table(_g_lock);
    LKM_DEBUG_PRINT("_g_lock_fail_time = %lld\n", _g_lock_fail_time);
    //release_lock(_g_lock, "/roy/MySync/aaa/ddd/", NULL);
    //print_lock_table(_g_lock);

    LKM_DEBUG_PRINT("start test destoty table[%p]\n", _g_lock);
    //destroy_lock_table(_g_lock);
    LKM_DEBUG_PRINT("_g_lock = %p\n", _g_lock);

    pthread_t lock_thread4;

    pthread_create(
        &lock_thread4,
        NULL, only_release_lock,
        NULL);
    pthread_join(lock_thread4, NULL);

    return 0;
}