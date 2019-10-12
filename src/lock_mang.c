#define _GNU_SOURCE
#include <pthread.h>
#include <unistd.h>
//#include "csg_operation.h"
#include "lock_keynode.h"
#include "lock_mang.h"
#include "errorno.h"
#include "macro.h"
#include "list.h"

#define fix_up_path(tmp_path_pchar, src_path_pchar) do{\
    strcpyALL(tmp_path_pchar, (char *)src_path_pchar);\
    if(tmp_path_pchar[strlen(src_path_pchar) -1] == '/')\
        tmp_path_pchar[strlen(src_path_pchar) - 1] = '\x0';\
}while(0);

/**
  * Hint :  Input data  is no be free in this func
  *
  * Using src_path_pchar/dest_path_pchar(if not NULL) to get lock
  * This method is blocking  function, waiting until success
  *
  * @param lock_table_pstruct type : tLockTable*
  *     Input the LockTable which want to reference
  * @param src_path_pchar type : const char*
  *     Input locking source path
  * @param dest_path_pchar type : const char*
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
    tLockTable *lock_table_pstruct,
    const char *src_path_pchar,
    const char *dest_path_pchar)
{
    //LKM_DEBUG_PRINT("start block lock\n");
    int ret_int = ERROR_CODE_LKM_ERROR;
    check_null_input(lock_table_pstruct);
    check_null_input(src_path_pchar);

    if( (!strcmp("/", src_path_pchar) ) ||
        ((NULL != dest_path_pchar) && !strcmp("/", dest_path_pchar)) )
        return ERROR_CODE_LKM_INPUT_FORMAT;

    if( __sync_fetch_and_or(&lock_table_pstruct->destroy_lock_int , 0))
    {
        LKM_DEBUG_PRINT("Table will be destroy, can't get lock");
        return ERROR_CODE_LKM_DESTROY;
    }
    __sync_fetch_and_add(&lock_table_pstruct->used_cnt_int, 1);

    char *tmp_path_pchar = NULL;
    fix_up_path(tmp_path_pchar, src_path_pchar);
#ifdef LKM_DEBUG_MODE
    LKM_DEBUG_PRINT("tmp_path_pchar = %s\n", tmp_path_pchar);
#endif
    while(ERROR_CODE_SUCCESS != ret_int)
    {
        ret_int = set_table_lock(tmp_path_pchar, lock_table_pstruct);
        if(ERROR_CODE_SUCCESS != ret_int)
            {
                sleep(1);
            }
    }

    free_to_NULL(tmp_path_pchar);

    if(NULL != dest_path_pchar)
    {
        char *tmp_path_pchar = NULL;
        fix_up_path(tmp_path_pchar, dest_path_pchar);
        while(ERROR_CODE_SUCCESS != ret_int)
        {
            ret_int = set_table_lock(tmp_path_pchar, lock_table_pstruct);
            if(ERROR_CODE_SUCCESS != ret_int)
            {
                sleep(1);
            }
        }

        free_to_NULL(tmp_path_pchar);
    }
    __sync_fetch_and_sub(&lock_table_pstruct->used_cnt_int, 1);
    return ERROR_CODE_SUCCESS;
}

/**
  * Hint :  input data  is no be free in this func
  *
  * Using src_path_pchar/dest_path_pchar(if not NULL) to get lock
  * If get lock fail will return ERROR_CODE_LKM_W_ERROR/ERROR_CODE_LKM_R_ERROR
  * for present why the path fail
  *
  * @param lock_table_pstruct type : tLockTable*
  *     Input the LockTable which want to reference
  * @param src_path_pchar type : const char*
  *     Input locking source path
  * @param dest_path_pchar type : const char*
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
    tLockTable *lock_table_pstruct,
    const char *src_path_pchar,
    const char *dest_path_pchar)
{
    //LKM_DEBUG_PRINT("start try lock\n");
    int ret_int = ERROR_CODE_SUCCESS;
    check_null_input(lock_table_pstruct);
    check_null_input(src_path_pchar);

    if( (!strcmp("/", src_path_pchar) ) ||
        ((NULL != dest_path_pchar) && !strcmp("/", dest_path_pchar)) )
        return ERROR_CODE_LKM_INPUT_FORMAT;

    if( __sync_fetch_and_or(&lock_table_pstruct->destroy_lock_int , 0))
    {
        LKM_DEBUG_PRINT("Table will be destroy, can't get lock");
        return ERROR_CODE_LKM_DESTROY;
    }
    __sync_fetch_and_add(&lock_table_pstruct->used_cnt_int, 1);

    char *tmp_path_pchar = NULL;
    fix_up_path(tmp_path_pchar, src_path_pchar);
#ifdef LKM_DEBUG_MODE
    LKM_DEBUG_PRINT("tmp_path_pchar = %s\n", tmp_path_pchar);
#endif
    ret_int = set_table_lock(tmp_path_pchar, lock_table_pstruct);
    free_to_NULL(tmp_path_pchar);
    if(ERROR_CODE_SUCCESS == ret_int)
    {
        if(NULL != dest_path_pchar)
        {
            fix_up_path(tmp_path_pchar, dest_path_pchar);
            ret_int = set_table_lock(tmp_path_pchar, lock_table_pstruct);
            free_to_NULL(tmp_path_pchar);
            if(ERROR_CODE_SUCCESS != ret_int)
            {
                fix_up_path(tmp_path_pchar, src_path_pchar);
                set_table_unlock(tmp_path_pchar, lock_table_pstruct);
            }
        }
    }
    __sync_fetch_and_sub(&lock_table_pstruct->used_cnt_int, 1);
    return ret_int;

}

/**
  * Hint :  input data  is no be free in this func
  *
  * Using src_path_pchar/dest_path_pchar(if not NULL) to release lock
  * This method is blocking func.
  *
  * @param lock_table_pstruct type : tLockTable*
  *     Input the LockTable which want to reference
  * @param src_path_pchar type : const char*
  *     Input locking source path
  * @param dest_path_pchar type : const char*
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
    tLockTable *lock_table_pstruct,
    const char *src_path_pchar,
    const char *dest_path_pchar)
{
    int ret_int = ERROR_CODE_SUCCESS;
    check_null_input(lock_table_pstruct);
    check_null_input(src_path_pchar);

    if( (!strcmp("/", src_path_pchar) ) ||
        ((NULL != dest_path_pchar) && !strcmp("/", dest_path_pchar)) )
        return ERROR_CODE_LKM_INPUT_FORMAT;

    if( __sync_fetch_and_or(&lock_table_pstruct->destroy_lock_int , 0))
    {
        LKM_DEBUG_PRINT("Table will be destroy, can't release lock");
        return ERROR_CODE_LKM_DESTROY;
    }
    __sync_fetch_and_add(&lock_table_pstruct->used_cnt_int, 1);
// release src path lock
    char *tmp_path_pchar = NULL;
    fix_up_path(tmp_path_pchar, src_path_pchar);
    ret_int = set_table_unlock(tmp_path_pchar, lock_table_pstruct);
    free_to_NULL(tmp_path_pchar);
// release dest path lock
    if(NULL != dest_path_pchar)
    {
        fix_up_path(tmp_path_pchar, dest_path_pchar);
        ret_int = set_table_unlock(tmp_path_pchar, lock_table_pstruct);
        free_to_NULL(tmp_path_pchar);
    }

    __sync_fetch_and_sub(&lock_table_pstruct->used_cnt_int, 1);


    return ret_int;

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
  * @param table_pstruct type : tLockTable*
  *     Input locking table address
  *
  * @return int
  *     ERROR_CODE_SUCCESS
  *     ERROR_CODE_NULL_POINT_EXCEPTION
  */
int destroy_lock_table(tLockTable *table_pstruct)
{
    check_null_input(table_pstruct);
    tLockTable *table = (tLockTable *)(table_pstruct);
    __sync_fetch_and_add(&table->destroy_lock_int, 1);
    while( __sync_fetch_and_or(&table->used_cnt_int , 0))
    {
        LKM_DEBUG_PRINT("someone used the lock table wait........");
        sleep(3);
    }

    struct list_head *nodeptr_pstruct = NULL;
    struct list_head *rootptr_pstruct = NULL;
    for(long long i = 0; i < MAX_LOCK_TABLE_INDEX; i++)
    {
        if(NULL != table->hash_table_pastruct[i])
        {
#ifdef LKM_DEBUG_MODE
            LKM_DEBUG_PRINT(
                "root node exist(%lld)(%p)\n",
                i,
                table->hash_table_pastruct[i]);
#endif
            rootptr_pstruct = table->hash_table_pastruct[i];
            list_for_each(nodeptr_pstruct, rootptr_pstruct)
            {
                tLockNode *listnode_pstruct = NULL;
                listnode_pstruct = list_entry(
                    nodeptr_pstruct,
                    tLockNode,
                    list_struct);
                nodeptr_pstruct = nodeptr_pstruct->next;
                list_del(&listnode_pstruct->list_struct);
#ifdef LKM_DEBUG_MODE
                LKM_DEBUG_PRINT(
                        "listnode_pstruct = %p\n"
                        "listnode_pstruct->name_pchar = %s\n"
                        "listnode_pstruct->read_lock = %ld\n"
                        "listnode_pstruct->write_lock = %d\n",
                        listnode_pstruct,
                        listnode_pstruct->name_pchar,
                        listnode_pstruct->read_lock,
                        listnode_pstruct->write_lock);
#endif
                free_to_NULL(listnode_pstruct->name_pchar);
                free_to_NULL(listnode_pstruct);
            }
            free_to_NULL(table->hash_table_pastruct[i]);
        }
    }

    free(table_pstruct);
    //(*table_puint64) = (unsigned long long )NULL;
    return ERROR_CODE_SUCCESS;
}

void print_lock_table(
    tLockTable *lock_struct)
{
    if(NULL == lock_struct) return;
    struct list_head *nodeptr_pstruct = NULL;
    struct list_head *rootptr_pstruct = NULL;
    uint64_t write_lock_int = 0;
    uint64_t read_lock_int = 0;
    //LKM_DEBUG_PRINT("PRINT table\n");
    for(uint64_t i = 0; i < MAX_LOCK_TABLE_INDEX; i++)
    {
        if(NULL != lock_struct->hash_table_pastruct[i])
        {

            LKM_DEBUG_PRINT(
                "root node exist(%ld)(%p)\n",
                i,
                lock_struct->hash_table_pastruct[i]);

            rootptr_pstruct = lock_struct->hash_table_pastruct[i];
            list_for_each(nodeptr_pstruct, rootptr_pstruct)
            {
                tLockNode *listnode_pstruct = NULL;
                listnode_pstruct = list_entry(
                    nodeptr_pstruct,
                    tLockNode,
                    list_struct);

                LKM_DEBUG_PRINT(
                    "search in node addr = %p\n",
                    listnode_pstruct);
                LKM_DEBUG_PRINT(
                    "listnode_pstruct->name_pchar = %s\n",
                    listnode_pstruct->name_pchar);
                LKM_DEBUG_PRINT(
                    "listnode_pstruct->read_lock = %ld\n",
                    listnode_pstruct->read_lock);
                LKM_DEBUG_PRINT(
                    "listnode_pstruct->write_lock = %d\n",
                    listnode_pstruct->write_lock);
                LKM_DEBUG_PRINT(
                    "listnode_pstruct->node_entry_lock_int = %d\n",
                    listnode_pstruct->node_entry_lock_int);

                read_lock_int += listnode_pstruct->read_lock;
                if(listnode_pstruct->write_lock)
                    write_lock_int++;
            }
        }
    }
    LKM_DEBUG_PRINT("write_lock_int = %ld\n", write_lock_int);
    LKM_DEBUG_PRINT("read_lock_int = %ld\n", read_lock_int);

}
