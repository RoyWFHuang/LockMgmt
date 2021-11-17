
#define _GNU_SOURCE
#include <pthread.h>
//#include <stdint.h>
#include <openssl/sha.h>
#define __GUNC__

#include "list.h"
#include "lock_keynode.h"
#include "lock_mang.h"
#include "errorno.h"
#include "macro.h"
#include "define.h"

#define get_first_key(index, tmp_src_path) do{\
    char *ptr1 = strstr(tmp_src_path, "/");\
    *ptr1 = ' '; \
    char *ptr_index = strstr(tmp_src_path, "/"); /* get first key*/ \
    if(NULL != ptr_index)\
        *ptr_index = '\x0';\
    *ptr1 = '/';\
    index = strlen(tmp_src_path);\
}while(0);

#if 0
#define get_first_key(index, tmp_src_path) do{\
    char *ptr1 = NULL, *ptr2 = NULL, \
    *ptr3 = NULL, *ptr_index;\
    /* clear "/user/MySync/path/to" first '/' to "user/MySync/path/to"*/\
    ptr1 = strstr(tmp_src_path, "/");\
    *ptr1 = ' '; \
    /* clear " user/MySync/path/to" first '/' to "user MySync/path/to"*/\
    ptr2 = strstr(tmp_src_path, "/");\
    *ptr2 = ' '; \
    /* clear " user MySync/path/to" first '/' to "user MySync path/to"*/ \
    ptr3 = strstr(tmp_src_path, "/");\
    *ptr3 = ' '; \
    ptr_index = strstr(tmp_src_path, "/"); /* get first key*/ \
    if(NULL != ptr_index)\
        *ptr_index = '\x0';\
    *ptr1 = '/';\
    *ptr2 = '/';\
    *ptr3 = '/';\
    index = strlen(tmp_src_path);\
}while(0);
#endif

static int32_t __get_hash_index(char *input)
{

    unsigned char sha[SHA_DIGEST_LENGTH];
	SHA_CTX c;
	SHA1_Init(&c);
	SHA1_Update(&c, input, strlen(input));
	SHA1_Final(sha, &c);
    //LKM_DEBUG_PRINT("%x %x %x %x\n", sha[0], sha[1],
    //    sha[SHA_DIGEST_LENGTH-2],
    //    sha[SHA_DIGEST_LENGTH-1]);
    int32_t hash_num =
        (sha[0] << 24) +
        (sha[1] << 16) +
        (sha[SHA_DIGEST_LENGTH-2] << 8) +
        sha[SHA_DIGEST_LENGTH-1];
    hash_num &= 0x7fffffff;
    //LKM_DEBUG_PRINT("hash_num = %d\n", hash_num);

    return (hash_num & ((1 << MAX_LOCK_TABLE_POWER_OF_2) -1));
}

static int __set_w_tLockNode(tLockNode *lock)
{
    int ret = ERROR_CODE_SUCCESS;
    if( !__sync_lock_test_and_set(&lock->node_entry_lock, 1))
    {
        //if(0 != lock->read_lock)
        if(0 != __sync_fetch_and_or(&lock->read_lock, 0))
        {
            LKM_DEBUG_PRINT("get w lock fail, read_lock[%ld]\n",
                lock->read_lock);
            ret = ERROR_CODE_LKM_W_ERROR;
            goto __set_w_tLockNode_exit_lab;
        }

        if(__sync_fetch_and_and(&lock->write_lock, 1))
        {
            LKM_DEBUG_PRINT("get w lock fail, write_lock[%d]\n",
                lock->write_lock);
            ret = ERROR_CODE_LKM_W_ERROR;
            goto __set_w_tLockNode_exit_lab;
        }
        __sync_fetch_and_or(&lock->write_lock, 1);
__set_w_tLockNode_exit_lab :
        __sync_lock_release(&lock->node_entry_lock);
    }
    else
    {
        ret =  ERROR_CODE_LKM_INTERNAL_ERR_GET_NODE_LOCK_FAIL;
    }
    return ret;
}

static int __set_r_tLockNode(tLockNode *lock)
{
    int ret = ERROR_CODE_SUCCESS;
    if( !__sync_lock_test_and_set(&lock->node_entry_lock, 1))
    {
        if(__sync_fetch_and_and(&lock->write_lock, 1))
        {
            ret = ERROR_CODE_LKM_R_ERROR;
            goto __set_r_tLockNode_exit_lab;
        }
        __sync_fetch_and_add(&lock->read_lock, 1);
__set_r_tLockNode_exit_lab :
        __sync_lock_release(&lock->node_entry_lock);
    }
    else
    {
        ret =  ERROR_CODE_LKM_INTERNAL_ERR_GET_NODE_LOCK_FAIL;
    }
    return ret;
}

static int __set_tLockNode(tLockNode *lock, eLockType locktype)
{
    int ret = ERROR_CODE_LKM_INTERNAL_ERR_GET_NODE_LOCK_FAIL;
    while(ERROR_CODE_LKM_INTERNAL_ERR_GET_NODE_LOCK_FAIL == ret){
        switch(locktype)
        {
            case eREAD_LOCK:
                ret = __set_r_tLockNode(lock);
                break;
            case eWRITE_LOCK:
                ret = __set_w_tLockNode(lock);
                break;
        }
    }
    return ret;
}

static int __unset_w_tLockNode(tLockNode *lock)
{
    int ret = ERROR_CODE_SUCCESS;
    if( !__sync_lock_test_and_set(&lock->node_entry_lock, 1))
    {
        __sync_fetch_and_and(&lock->write_lock, 0);
        __sync_lock_release(&lock->node_entry_lock);
    }
    else
    {
        ret = ERROR_CODE_LKM_INTERNAL_ERR_GET_NODE_LOCK_FAIL;
    }
    return ret;
}

static int __unset_r_tLockNode(tLockNode *lock)
{
    int ret = ERROR_CODE_SUCCESS;
    if( !__sync_lock_test_and_set(&lock->node_entry_lock, 1))
    {
        __sync_fetch_and_sub(&lock->read_lock, 1);
        __sync_lock_release(&lock->node_entry_lock);
    }
    else
    {
        ret = ERROR_CODE_LKM_INTERNAL_ERR_GET_NODE_LOCK_FAIL;
    }
    return ret;
}

static int __unset_tLockNode(tLockNode *lock, eLockType locktype)
{
    int ret = ERROR_CODE_LKM_INTERNAL_ERR_GET_NODE_LOCK_FAIL;
    while(ERROR_CODE_LKM_INTERNAL_ERR_GET_NODE_LOCK_FAIL == ret){
        switch(locktype)
        {
            case eREAD_LOCK:
                ret = __unset_r_tLockNode(lock);
                break;
            case eWRITE_LOCK:
                ret = __unset_w_tLockNode(lock);
                break;
        }
    }
    return ret;
}

static int __serch_and_set_tLockNode(
    const char *src_path,
    struct list_head **lock,
    eLockType locktype)
{
    int ret;
    struct list_head *nodeptr = NULL;
    struct list_head *rootptr = NULL;

    if(likely(NULL != (*lock) ) )
    {
        rootptr = (*lock);
        list_for_each(nodeptr, rootptr)
        {
            tLockNode *listnode = NULL;
            listnode = list_entry(
                nodeptr,
                tLockNode,
                list_struct);
            //LKM_DEBUG_PRINT(
            //    "search in node addr = %p\n",
            //    listnode);
            if(!strcmp(listnode->name, src_path))
            {
                ret = __set_tLockNode(listnode, locktype);
                return ret;
            }
        }
    }
    else
    {
        (*lock) = calloc(1, sizeof(struct list_head));
        INIT_LIST_HEAD((*lock));
        rootptr = (*lock);
        //LKM_DEBUG_PRINT("root lock = %p\n",rootptr);
    }
    //LKM_DEBUG_PRINT("rootptr = %p\n",rootptr);
    //LKM_DEBUG_PRINT("not find create a new node\n");
    tLockNode *new_lock_node = calloc(1, sizeof(tLockNode));
    strcpyALL(new_lock_node->name, (char *)src_path);
    ret = __set_tLockNode(new_lock_node, locktype);
    if(likely(NULL != rootptr) )
        list_add(&new_lock_node->list_struct, rootptr);
    return ret;
}

static int __serch_and_unset_tLockNode(
    const char *src_path,
    struct list_head *lock,
    eLockType locktype)
{
    int ret = ERROR_CODE_LKM_INTERNAL_SEARCH_NOT_FOUND;
    if(NULL == lock)
        goto __serch_and_unset_tLockNode_exist_lab;
    struct list_head *nodeptr = NULL;
    struct list_head *rootptr = lock;
    list_for_each(nodeptr, rootptr)
    {
        tLockNode *listnode = NULL;
        listnode = list_entry(nodeptr, tLockNode, list_struct);
        //LKM_DEBUG_PRINT(
        //    "search in node addr = %p\n",
        //    listnode);
        if(!strcmp(listnode->name, src_path))
        {
            ret = __unset_tLockNode(listnode, locktype);
            //int remove_flag = 0;
            // check if the node need free
            if(!__sync_lock_test_and_set(
                &listnode->node_entry_lock, 1))
            {
                if( (0 == listnode->read_lock) &&
                    (0 == listnode->read_lock))
                {
                    //remove_flag = 1;
                    list_del(&listnode->list_struct);
#ifdef LKM_DEBUG_MODE
                    LKM_DEBUG_PRINT("free this node[%s](%p) ",
                        listnode->name,
                        listnode);
#endif
                    free_to_NULL(listnode->name);
                    free_to_NULL(listnode);
                }
                else
                    __sync_lock_release(&listnode->node_entry_lock);
            }
            /*
            if(remove_flag)
            {
                free_to_NULL(listnode->name);
                free_to_NULL(listnode);
            }
            */
            return ret;
        }
    }
__serch_and_unset_tLockNode_exist_lab :
    return ret;
}



static int __serch_and_set_locktable(
    const char *src_path,
    const int end_index,
    tLockTable *lock_table)
{
    int ret = ERROR_CODE_SUCCESS;
    struct list_head **hash_table_ppstruct =
        lock_table->hash_table;

    uint8_t *entry_lock_pint = lock_table->hash_table_entry_lock_aint;

    char *tmp_src_path = NULL;
    strcpyALL(tmp_src_path, (char *)src_path);
    tmp_src_path[end_index] = '\x0';

    int32_t index = __get_hash_index(tmp_src_path);

    eLockType locktype;
    if(strlen(src_path) == end_index)
        locktype = eWRITE_LOCK;
    else
        locktype = eREAD_LOCK;
#ifdef LKM_DEBUG_MODE
    LKM_DEBUG_PRINT("tmp_src_path = %s, index = %d, \
locktype = %d\n",
        tmp_src_path,
        index,
        locktype );
#endif
    int flag = 0;
    while(!flag)
    {
        if( !__sync_lock_test_and_set(&entry_lock_pint[index], 1))
        {
            //LKM_DEBUG_PRINT("get list lock ,"
            //"&hash_table_ppstruct[index] = %p, "
            //"entry_lock_pint[index] = %d\n",
            //    &hash_table_ppstruct[index],
            //    entry_lock_pint[index]);

            flag = 1;
            ret = __serch_and_set_tLockNode(
                tmp_src_path,
                &hash_table_ppstruct[index],
                locktype);
            __sync_lock_release(&entry_lock_pint[index]);
        }
    }

    if((ERROR_CODE_SUCCESS == ret) &&
        (eREAD_LOCK == locktype)) // set success, try to get next lock
    {
        char *nextstart = &tmp_src_path[end_index+1];
        tmp_src_path[end_index] = '/';
        char *nextstr = strstr(nextstart, "/");
        if(NULL != nextstr)
            *nextstr = '\x0';

        int next_index = strlen(tmp_src_path);
        ret = __serch_and_set_locktable(
                    src_path,
                    next_index,
                    lock_table);
        // set next lock fail, release this lock
        if(ERROR_CODE_SUCCESS != ret)
        {
            tmp_src_path[end_index] = '\x0';
            flag = 0;
            while(!flag)
            {
                if( !__sync_lock_test_and_set(&entry_lock_pint[index], 1))
                {
                    flag = 1;
                    __serch_and_unset_tLockNode(
                        tmp_src_path,
                        hash_table_ppstruct[index],
                        locktype);
                    __sync_lock_release(&entry_lock_pint[index]);
                }
            }
            tmp_src_path[end_index] = '/';
        }
    }
    free_to_NULL(tmp_src_path);
    return ret;
}


static int __serch_and_unset_locktable(
    const char *src_path,
    const int end_index,
    tLockTable *lock_table)
{
    int ret = ERROR_CODE_SUCCESS;
    //tLockNode **hash_table_ppstruct = lock_table->hash_table;
    struct list_head **hash_table_ppstruct =
        lock_table->hash_table;
    uint8_t *entry_lock_pint = lock_table->hash_table_entry_lock_aint;

    char *tmp_src_path = NULL;
    strcpyALL(tmp_src_path, (char *)src_path);
    tmp_src_path[end_index] = '\x0';

    int index = __get_hash_index(tmp_src_path);


    eLockType locktype;
    if(strlen(src_path) == end_index)
        locktype = eWRITE_LOCK;
    else
        locktype = eREAD_LOCK;
#ifdef LKM_DEBUG_MODE
    LKM_DEBUG_PRINT("tmp_src_path = %s, index = %d, \
locktype = %d\n",
        tmp_src_path,
        index,
        locktype );
#endif
    int flag = 0;
    while(!flag)
    {
        if( !__sync_lock_test_and_set(&entry_lock_pint[index], 1))
        {
            flag = 1;
            ret = __serch_and_unset_tLockNode(
                tmp_src_path,
                hash_table_ppstruct[index],
                locktype);
            __sync_lock_release(&entry_lock_pint[index]);
        }
    }

    if((ERROR_CODE_SUCCESS == ret) &&
        (eREAD_LOCK == locktype)) // set success, try to get next lock
    {
        char *nextstart = &tmp_src_path[end_index+1];
        tmp_src_path[end_index] = '/';
        char *nextstr = strstr(nextstart, "/");
        if(NULL != nextstr)
            *nextstr = '\x0';
        int next_index = strlen(tmp_src_path);
        ret = __serch_and_unset_locktable(
                    src_path,
                    next_index,
                    lock_table);
        // unset next lock fail, add this lock
        if(ERROR_CODE_SUCCESS != ret)
        {
            tmp_src_path[end_index] = '\x0';
            flag = 0;
            while(!flag)
            {
                if( !__sync_lock_test_and_set(&entry_lock_pint[index], 1))
                {
                    flag = 1;
                    __serch_and_set_tLockNode(
                        tmp_src_path,
                        &hash_table_ppstruct[index],
                        locktype);
                    __sync_lock_release(&entry_lock_pint[index]);
                }
            }
            tmp_src_path[end_index] = '/';
        }
    }
    free_to_NULL(tmp_src_path);
    return ret;
}


int set_table_lock(
    char *src_path,
    tLockTable *lock_table)
{
    int end_index = 0;
    char *tmp_src_path = NULL;
    strcpyALL(tmp_src_path, src_path);
    get_first_key(end_index, tmp_src_path);
    //LKM_DEBUG_PRINT("tmp_src_path = %s, end_index = %d\n",
    //    tmp_src_path,
    //    end_index);
    free_to_NULL(tmp_src_path);

    return __serch_and_set_locktable(
        src_path,
        end_index,
        lock_table);
}


int set_table_unlock(
    char *src_path,
    tLockTable *lock_table)
{
    int end_index = 0;
    char *tmp_src_path = NULL;
    strcpyALL(tmp_src_path, src_path);
    get_first_key(end_index, tmp_src_path);
    //LKM_DEBUG_PRINT("tmp_src_path = %s, end_index = %d\n",
    //    tmp_src_path,
    //    end_index);
    free_to_NULL(tmp_src_path);

    return __serch_and_unset_locktable(
        src_path,
        end_index,
        lock_table);
}
