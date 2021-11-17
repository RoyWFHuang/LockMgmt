#define _GNU_SOURCE
#include <pthread.h>
#include <unistd.h>
//#include "csg_operation.h"
#include "lock_keynode.h"
#include "lock_mang.h"
#include "errorno.h"
#include "macro.h"
#include "list.h"

#define fix_up_path(tmp_path, src_path) do{\
    strcpyALL(tmp_path, (char *)src_path);\
    if(tmp_path[strlen(src_path) -1] == '/')\
        tmp_path[strlen(src_path) - 1] = '\x0';\
}while(0);

/**
  * Hint :  Input data  is no be free in this func
  *
  * Using src_path/dest_path(if not NULL) to get lock
  * This method is blocking  function, waiting until success
  *
  * @param lock_table type : tLockTable*
  *     Input the LockTable which want to reference
  * @param src_path type : const char*
  *     Input locking source path
  * @param dest_path type : const char*
  *     Input locking destination path, if exist
  *
  * @return int
  *     ERROR_CODE_SUCCESS
  *     ERROR_CODE_LKM_INPUT_FORMAT
  *     ERROR_CODE_LKM_DESTROY
  *     ERROR_CODE_NULL_POINT_EXCEPTION
  *	    ERROR_CODE_NONEXPECT_ERROR
  */
int block_lock(
    tLockTable *lock_table,
    const char *src_path,
    const char *dest_path)
{
    //LKM_DEBUG_PRINT("start block lock\n");
    int ret = ERROR_CODE_LKM_ERROR;
    check_null_input(lock_table);
    check_null_input(src_path);

    if( (!strcmp("/", src_path) ) ||
        ((NULL != dest_path) && !strcmp("/", dest_path)) )
        return ERROR_CODE_LKM_INPUT_FORMAT;

    if( __sync_fetch_and_or(&lock_table->destroy_lock , 0))
    {
        LKM_DEBUG_PRINT("Table will be destroy, can't get lock");
        return ERROR_CODE_LKM_DESTROY;
    }
    __sync_fetch_and_add(&lock_table->used_cnt, 1);

    char *tmp_path = NULL;
    fix_up_path(tmp_path, src_path);
#ifdef LKM_DEBUG_MODE
    LKM_DEBUG_PRINT("tmp_path = %s\n", tmp_path);
#endif
    while(ERROR_CODE_SUCCESS != ret)
    {
        ret = set_table_lock(tmp_path, lock_table);
        if(ERROR_CODE_SUCCESS != ret)
            {
                sleep(1);
            }
    }

    free_to_NULL(tmp_path);

    if(NULL != dest_path)
    {
        char *tmp_path = NULL;
        fix_up_path(tmp_path, dest_path);
        while(ERROR_CODE_SUCCESS != ret)
        {
            ret = set_table_lock(tmp_path, lock_table);
            if(ERROR_CODE_SUCCESS != ret)
            {
                sleep(1);
            }
        }

        free_to_NULL(tmp_path);
    }
    __sync_fetch_and_sub(&lock_table->used_cnt, 1);
    return ERROR_CODE_SUCCESS;
}

/**
  * Hint :  input data  is no be free in this func
  *
  * Using src_path/dest_path(if not NULL) to get lock
  * If get lock fail will return ERROR_CODE_LKM_W_ERROR/ERROR_CODE_LKM_R_ERROR
  * for present why the path fail
  *
  * @param lock_table type : tLockTable*
  *     Input the LockTable which want to reference
  * @param src_path type : const char*
  *     Input locking source path
  * @param dest_path type : const char*
  *     Input locking destination path, if exist
  *
  * @return int
  *     ERROR_CODE_SUCCESS
  *     ERROR_CODE_LKM_W_ERROR
  *     ERROR_CODE_LKM_R_ERROR
  *     ERROR_CODE_LKM_DESTROY
  *     ERROR_CODE_LKM_INPUT_FORMAT
  *     ERROR_CODE_NULL_POINT_EXCEPTION
  *	    ERROR_CODE_NONEXPECT_ERROR
  */
int try_lock(
    tLockTable *lock_table,
    const char *src_path,
    const char *dest_path)
{
    //LKM_DEBUG_PRINT("start try lock\n");
    int ret = ERROR_CODE_SUCCESS;
    check_null_input(lock_table);
    check_null_input(src_path);

    if( (!strcmp("/", src_path) ) ||
        ((NULL != dest_path) && !strcmp("/", dest_path)) )
        return ERROR_CODE_LKM_INPUT_FORMAT;

    if( __sync_fetch_and_or(&lock_table->destroy_lock , 0))
    {
        LKM_DEBUG_PRINT("Table will be destroy, can't get lock");
        return ERROR_CODE_LKM_DESTROY;
    }
    __sync_fetch_and_add(&lock_table->used_cnt, 1);

    char *tmp_path = NULL;
    fix_up_path(tmp_path, src_path);
#ifdef LKM_DEBUG_MODE
    LKM_DEBUG_PRINT("tmp_path = %s\n", tmp_path);
#endif
    ret = set_table_lock(tmp_path, lock_table);
    free_to_NULL(tmp_path);
    if(ERROR_CODE_SUCCESS == ret)
    {
        if(NULL != dest_path)
        {
            fix_up_path(tmp_path, dest_path);
            ret = set_table_lock(tmp_path, lock_table);
            free_to_NULL(tmp_path);
            if(ERROR_CODE_SUCCESS != ret)
            {
                fix_up_path(tmp_path, src_path);
                set_table_unlock(tmp_path, lock_table);
            }
        }
    }
    __sync_fetch_and_sub(&lock_table->used_cnt, 1);
    return ret;

}

/**
  * Hint :  input data  is no be free in this func
  *
  * Using src_path/dest_path(if not NULL) to release lock
  * This method is blocking func.
  *
  * @param lock_table type : tLockTable*
  *     Input the LockTable which want to reference
  * @param src_path type : const char*
  *     Input locking source path
  * @param dest_path type : const char*
  *     Input locking destination path, if exist
  *
  * @return int
  *     ERROR_CODE_SUCCESS
  *     ERROR_CODE_LKM_DESTROY
  *     ERROR_CODE_LKM_INPUT_FORMAT
  *     ERROR_CODE_NULL_POINT_EXCEPTION
  *	    ERROR_CODE_NONEXPECT_ERROR
  */
int release_lock(
    tLockTable *lock_table,
    const char *src_path,
    const char *dest_path)
{
    int ret = ERROR_CODE_SUCCESS;
    check_null_input(lock_table);
    check_null_input(src_path);

    if( (!strcmp("/", src_path) ) ||
        ((NULL != dest_path) && !strcmp("/", dest_path)) )
        return ERROR_CODE_LKM_INPUT_FORMAT;

    if( __sync_fetch_and_or(&lock_table->destroy_lock , 0))
    {
        LKM_DEBUG_PRINT("Table will be destroy, can't release lock");
        return ERROR_CODE_LKM_DESTROY;
    }
    __sync_fetch_and_add(&lock_table->used_cnt, 1);
// release src path lock
    char *tmp_path = NULL;
    fix_up_path(tmp_path, src_path);
    ret = set_table_unlock(tmp_path, lock_table);
    free_to_NULL(tmp_path);
// release dest path lock
    if(NULL != dest_path)
    {
        fix_up_path(tmp_path, dest_path);
        ret = set_table_unlock(tmp_path, lock_table);
        free_to_NULL(tmp_path);
    }

    __sync_fetch_and_sub(&lock_table->used_cnt, 1);


    return ret;

}

/**
  * Hint :  input data  is no be free in this func
  *
  * Get initail lock table for locking use
  *
  * @return tLockTable*
  *     return create lock table address
  */
tLockTable *initial_lock_table()
{
    tLockTable *table = calloc(1, sizeof(tLockTable));
    return table;
}

/**
  * Hint :  input data  is "be free" in this func
  *
  * This function will destroy the lock table and set the point to null
  *
  * @param table type : tLockTable*
  *     Input locking table address
  *
  * @return int
  *     ERROR_CODE_SUCCESS
  *     ERROR_CODE_NULL_POINT_EXCEPTION
  */
int destroy_lock_table(tLockTable *table)
{
    check_null_input(table);
    tLockTable *tmp_table = (tLockTable *)(table);
    __sync_fetch_and_add(&table->destroy_lock, 1);
    while( __sync_fetch_and_or(&table->used_cnt , 0))
    {
        LKM_DEBUG_PRINT("someone used the lock table wait........");
        sleep(3);
    }

    struct list_head *nodeptr = NULL;
    struct list_head *rootptr = NULL;
    for(long long i = 0; i < MAX_LOCK_TABLE_INDEX; i++)
    {
        if(NULL != table->hash_table[i])
        {
#ifdef LKM_DEBUG_MODE
            LKM_DEBUG_PRINT(
                "root node exist(%lld)(%p)\n",
                i,
                table->hash_table[i]);
#endif
            rootptr = table->hash_table[i];
            list_for_each(nodeptr, rootptr)
            {
                tLockNode *listnode = NULL;
                listnode = list_entry(
                    nodeptr,
                    tLockNode,
                    list_struct);
                nodeptr = nodeptr->next;
                list_del(&listnode->list_struct);
#ifdef LKM_DEBUG_MODE
                LKM_DEBUG_PRINT(
                        "listnode = %p\n"
                        "listnode->name = %s\n"
                        "listnode->read_lock = %ld\n"
                        "listnode->write_lock = %d\n",
                        listnode,
                        listnode->name,
                        listnode->read_lock,
                        listnode->write_lock);
#endif
                free_to_NULL(listnode->name);
                free_to_NULL(listnode);
            }
            free_to_NULL(table->hash_table[i]);
        }
    }

    free(table);
    //(*table_puint64) = (unsigned long long )NULL;
    return ERROR_CODE_SUCCESS;
}

void print_lock_table(
    tLockTable *lock_struct)
{
    if(NULL == lock_struct) return;
    struct list_head *nodeptr = NULL;
    struct list_head *rootptr = NULL;
    uint64_t write_lock = 0;
    uint64_t read_lock = 0;
    //LKM_DEBUG_PRINT("PRINT table\n");
    for(uint64_t i = 0; i < MAX_LOCK_TABLE_INDEX; i++)
    {
        if(NULL != lock_struct->hash_table[i])
        {

            LKM_DEBUG_PRINT(
                "root node exist(%ld)(%p)\n",
                i,
                lock_struct->hash_table[i]);

            rootptr = lock_struct->hash_table[i];
            list_for_each(nodeptr, rootptr)
            {
                tLockNode *listnode = NULL;
                listnode = list_entry(
                    nodeptr,
                    tLockNode,
                    list_struct);

                LKM_DEBUG_PRINT(
                    "search in node addr = %p\n",
                    listnode);
                LKM_DEBUG_PRINT(
                    "listnode->name = %s\n",
                    listnode->name);
                LKM_DEBUG_PRINT(
                    "listnode->read_lock = %ld\n",
                    listnode->read_lock);
                LKM_DEBUG_PRINT(
                    "listnode->write_lock = %d\n",
                    listnode->write_lock);
                LKM_DEBUG_PRINT(
                    "listnode->node_entry_lock = %d\n",
                    listnode->node_entry_lock);

                read_lock += listnode->read_lock;
                if(listnode->write_lock)
                    write_lock++;
            }
        }
    }
    LKM_DEBUG_PRINT("write_lock = %ld\n", write_lock);
    LKM_DEBUG_PRINT("read_lock = %ld\n", read_lock);

}
