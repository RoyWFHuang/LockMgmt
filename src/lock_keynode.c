
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

#define get_first_key(index_int, tmp_src_path_pchar) do{\
    char *ptr1_pchar = strstr(tmp_src_path_pchar, "/");\
    *ptr1_pchar = ' '; \
    char *ptr_index_pchar = strstr(tmp_src_path_pchar, "/"); /* get first key*/ \
    if(NULL != ptr_index_pchar)\
        *ptr_index_pchar = '\x0';\
    *ptr1_pchar = '/';\
    index_int = strlen(tmp_src_path_pchar);\
}while(0);

#if 0
#define get_first_key(index_int, tmp_src_path_pchar) do{\
    char *ptr1_pchar = NULL, *ptr2_pchar = NULL, \
    *ptr3_pchar = NULL, *ptr_index_pchar;\
    /* clear "/user/MySync/path/to" first '/' to "user/MySync/path/to"*/\
    ptr1_pchar = strstr(tmp_src_path_pchar, "/");\
    *ptr1_pchar = ' '; \
    /* clear " user/MySync/path/to" first '/' to "user MySync/path/to"*/\
    ptr2_pchar = strstr(tmp_src_path_pchar, "/");\
    *ptr2_pchar = ' '; \
    /* clear " user MySync/path/to" first '/' to "user MySync path/to"*/ \
    ptr3_pchar = strstr(tmp_src_path_pchar, "/");\
    *ptr3_pchar = ' '; \
    ptr_index_pchar = strstr(tmp_src_path_pchar, "/"); /* get first key*/ \
    if(NULL != ptr_index_pchar)\
        *ptr_index_pchar = '\x0';\
    *ptr1_pchar = '/';\
    *ptr2_pchar = '/';\
    *ptr3_pchar = '/';\
    index_int = strlen(tmp_src_path_pchar);\
}while(0);
#endif

static int32_t __get_hash_index(char *input_pchar)
{

    unsigned char sha_achar[SHA_DIGEST_LENGTH];
	SHA_CTX c;
	SHA1_Init(&c);
	SHA1_Update(&c, input_pchar, strlen(input_pchar));
	SHA1_Final(sha_achar, &c);
    //LKM_DEBUG_PRINT("%x %x %x %x\n", sha_achar[0], sha_achar[1],
    //    sha_achar[SHA_DIGEST_LENGTH-2],
    //    sha_achar[SHA_DIGEST_LENGTH-1]);
    int32_t hash_num =
        (sha_achar[0] << 24) +
        (sha_achar[1] << 16) +
        (sha_achar[SHA_DIGEST_LENGTH-2] << 8) +
        sha_achar[SHA_DIGEST_LENGTH-1];
    hash_num &= 0x7fffffff;
    //LKM_DEBUG_PRINT("hash_num = %d\n", hash_num);

    return (hash_num & ((1 << MAX_LOCK_TABLE_POWER_OF_2) -1));
}

static int __set_w_tLockNode(tLockNode *lock)
{
    int ret_int = ERROR_CODE_SUCCESS;
    if( !__sync_lock_test_and_set(&lock->node_entry_lock_int, 1))
    {
        //if(0 != lock->read_lock)
        if(0 != __sync_fetch_and_or(&lock->read_lock, 0))
        {
            LKM_DEBUG_PRINT("get w lock fail, read_lock[%ld]\n",
                lock->read_lock);
            ret_int = ERROR_CODE_LKM_W_ERROR;
            goto __set_w_tLockNode_exit_lab;
        }

        if(__sync_fetch_and_and(&lock->write_lock, 1))
        {
            LKM_DEBUG_PRINT("get w lock fail, write_lock[%d]\n",
                lock->write_lock);
            ret_int = ERROR_CODE_LKM_W_ERROR;
            goto __set_w_tLockNode_exit_lab;
        }
        __sync_fetch_and_or(&lock->write_lock, 1);
__set_w_tLockNode_exit_lab :
        __sync_lock_release(&lock->node_entry_lock_int);
    }
    else
    {
        ret_int =  ERROR_CODE_LKM_INTERNAL_ERR_GET_NODE_LOCK_FAIL;
    }
    return ret_int;
}

static int __set_r_tLockNode(tLockNode *lock)
{
    int ret_int = ERROR_CODE_SUCCESS;
    if( !__sync_lock_test_and_set(&lock->node_entry_lock_int, 1))
    {
        if(__sync_fetch_and_and(&lock->write_lock, 1))
        {
            ret_int = ERROR_CODE_LKM_R_ERROR;
            goto __set_r_tLockNode_exit_lab;
        }
        __sync_fetch_and_add(&lock->read_lock, 1);
__set_r_tLockNode_exit_lab :
        __sync_lock_release(&lock->node_entry_lock_int);
    }
    else
    {
        ret_int =  ERROR_CODE_LKM_INTERNAL_ERR_GET_NODE_LOCK_FAIL;
    }
    return ret_int;
}

static int __set_tLockNode(tLockNode *lock, eLockType locktype_int)
{
    int ret_int = ERROR_CODE_LKM_INTERNAL_ERR_GET_NODE_LOCK_FAIL;
    while(ERROR_CODE_LKM_INTERNAL_ERR_GET_NODE_LOCK_FAIL == ret_int){
        switch(locktype_int)
        {
            case eREAD_LOCK:
                ret_int = __set_r_tLockNode(lock);
                break;
            case eWRITE_LOCK:
                ret_int = __set_w_tLockNode(lock);
                break;
        }
    }
    return ret_int;
}

static int __unset_w_tLockNode(tLockNode *lock)
{
    int ret_int = ERROR_CODE_SUCCESS;
    if( !__sync_lock_test_and_set(&lock->node_entry_lock_int, 1))
    {
        __sync_fetch_and_and(&lock->write_lock, 0);
        __sync_lock_release(&lock->node_entry_lock_int);
    }
    else
    {
        ret_int = ERROR_CODE_LKM_INTERNAL_ERR_GET_NODE_LOCK_FAIL;
    }
    return ret_int;
}

static int __unset_r_tLockNode(tLockNode *lock)
{
    int ret_int = ERROR_CODE_SUCCESS;
    if( !__sync_lock_test_and_set(&lock->node_entry_lock_int, 1))
    {
        __sync_fetch_and_sub(&lock->read_lock, 1);
        __sync_lock_release(&lock->node_entry_lock_int);
    }
    else
    {
        ret_int = ERROR_CODE_LKM_INTERNAL_ERR_GET_NODE_LOCK_FAIL;
    }
    return ret_int;
}

static int __unset_tLockNode(tLockNode *lock, eLockType locktype_int)
{
    int ret_int = ERROR_CODE_LKM_INTERNAL_ERR_GET_NODE_LOCK_FAIL;
    while(ERROR_CODE_LKM_INTERNAL_ERR_GET_NODE_LOCK_FAIL == ret_int){
        switch(locktype_int)
        {
            case eREAD_LOCK:
                ret_int = __unset_r_tLockNode(lock);
                break;
            case eWRITE_LOCK:
                ret_int = __unset_w_tLockNode(lock);
                break;
        }
    }
    return ret_int;
}

static int __serch_and_set_tLockNode(
    const char *src_path_pchar,
    struct list_head **lock,
    eLockType locktype_int)
{
    int ret_int;
    struct list_head *nodeptr_pstruct = NULL;
    struct list_head *rootptr_pstruct = NULL;

    if(likely(NULL != (*lock) ) )
    {
        rootptr_pstruct = (*lock);
        list_for_each(nodeptr_pstruct, rootptr_pstruct)
        {
            tLockNode *listnode_pstruct = NULL;
            listnode_pstruct = list_entry(
                nodeptr_pstruct,
                tLockNode,
                list_struct);
            //LKM_DEBUG_PRINT(
            //    "search in node addr = %p\n",
            //    listnode_pstruct);
            if(!strcmp(listnode_pstruct->name_pchar, src_path_pchar))
            {
                ret_int = __set_tLockNode(listnode_pstruct, locktype_int);
                return ret_int;
            }
        }
    }
    else
    {
        (*lock) = calloc(1, sizeof(struct list_head));
        INIT_LIST_HEAD((*lock));
        rootptr_pstruct = (*lock);
        //LKM_DEBUG_PRINT("root lock = %p\n",rootptr_pstruct);
    }
    //LKM_DEBUG_PRINT("rootptr_pstruct = %p\n",rootptr_pstruct);
    //LKM_DEBUG_PRINT("not find create a new node\n");
    tLockNode *new_lock_node_pstruct = calloc(1, sizeof(tLockNode));
    strcpyALL(new_lock_node_pstruct->name_pchar, (char *)src_path_pchar);
    ret_int = __set_tLockNode(new_lock_node_pstruct, locktype_int);
    if(likely(NULL != rootptr_pstruct) )
        list_add(&new_lock_node_pstruct->list_struct, rootptr_pstruct);
    return ret_int;
}

static int __serch_and_unset_tLockNode(
    const char *src_path_pchar,
    struct list_head *lock,
    eLockType locktype_int)
{
    int ret_int = ERROR_CODE_LKM_INTERNAL_SEARCH_NOT_FOUND;
    if(NULL == lock)
        goto __serch_and_unset_tLockNode_exist_lab;
    struct list_head *nodeptr_pstruct = NULL;
    struct list_head *rootptr_pstruct = lock;
    list_for_each(nodeptr_pstruct, rootptr_pstruct)
    {
        tLockNode *listnode_pstruct = NULL;
        listnode_pstruct = list_entry(nodeptr_pstruct, tLockNode, list_struct);
        //LKM_DEBUG_PRINT(
        //    "search in node addr = %p\n",
        //    listnode_pstruct);
        if(!strcmp(listnode_pstruct->name_pchar, src_path_pchar))
        {
            ret_int = __unset_tLockNode(listnode_pstruct, locktype_int);
            //int remove_flag = 0;
            // check if the node need free
            if(!__sync_lock_test_and_set(
                &listnode_pstruct->node_entry_lock_int, 1))
            {
                if( (0 == listnode_pstruct->read_lock) &&
                    (0 == listnode_pstruct->read_lock))
                {
                    //remove_flag = 1;
                    list_del(&listnode_pstruct->list_struct);
#ifdef LKM_DEBUG_MODE
                    LKM_DEBUG_PRINT("free this node[%s](%p) ",
                        listnode_pstruct->name_pchar,
                        listnode_pstruct);
#endif
                    free_to_NULL(listnode_pstruct->name_pchar);
                    free_to_NULL(listnode_pstruct);
                }
                else
                    __sync_lock_release(&listnode_pstruct->node_entry_lock_int);
            }
            /*
            if(remove_flag)
            {
                free_to_NULL(listnode_pstruct->name_pchar);
                free_to_NULL(listnode_pstruct);
            }
            */
            return ret_int;
        }
    }
__serch_and_unset_tLockNode_exist_lab :
    return ret_int;
}



static int __serch_and_set_locktable(
    const char *src_path_pchar,
    const int end_index_int,
    tLockTable *lock_table_pstruct)
{
    int ret_int = ERROR_CODE_SUCCESS;
    struct list_head **hash_table_ppstruct =
        lock_table_pstruct->hash_table_pastruct;

    uint8_t *entry_lock_pint = lock_table_pstruct->hash_table_entry_lock_aint;

    char *tmp_src_path_pchar = NULL;
    strcpyALL(tmp_src_path_pchar, (char *)src_path_pchar);
    tmp_src_path_pchar[end_index_int] = '\x0';

    int32_t index_int = __get_hash_index(tmp_src_path_pchar);

    eLockType locktype_int;
    if(strlen(src_path_pchar) == end_index_int)
        locktype_int = eWRITE_LOCK;
    else
        locktype_int = eREAD_LOCK;
#ifdef LKM_DEBUG_MODE
    LKM_DEBUG_PRINT("tmp_src_path_pchar = %s, index_int = %d, \
locktype_int = %d\n",
        tmp_src_path_pchar,
        index_int,
        locktype_int );
#endif
    int flag_int = 0;
    while(!flag_int)
    {
        if( !__sync_lock_test_and_set(&entry_lock_pint[index_int], 1))
        {
            //LKM_DEBUG_PRINT("get list lock ,"
            //"&hash_table_ppstruct[index_int] = %p, "
            //"entry_lock_pint[index_int] = %d\n",
            //    &hash_table_ppstruct[index_int],
            //    entry_lock_pint[index_int]);

            flag_int = 1;
            ret_int = __serch_and_set_tLockNode(
                tmp_src_path_pchar,
                &hash_table_ppstruct[index_int],
                locktype_int);
            __sync_lock_release(&entry_lock_pint[index_int]);
        }
    }

    if((ERROR_CODE_SUCCESS == ret_int) &&
        (eREAD_LOCK == locktype_int)) // set success, try to get next lock
    {
        char *nextstart_pchar = &tmp_src_path_pchar[end_index_int+1];
        tmp_src_path_pchar[end_index_int] = '/';
        char *nextstr_pchar = strstr(nextstart_pchar, "/");
        if(NULL != nextstr_pchar)
            *nextstr_pchar = '\x0';

        int next_index_int = strlen(tmp_src_path_pchar);
        ret_int = __serch_and_set_locktable(
                    src_path_pchar,
                    next_index_int,
                    lock_table_pstruct);
        // set next lock fail, release this lock
        if(ERROR_CODE_SUCCESS != ret_int)
        {
            tmp_src_path_pchar[end_index_int] = '\x0';
            flag_int = 0;
            while(!flag_int)
            {
                if( !__sync_lock_test_and_set(&entry_lock_pint[index_int], 1))
                {
                    flag_int = 1;
                    __serch_and_unset_tLockNode(
                        tmp_src_path_pchar,
                        hash_table_ppstruct[index_int],
                        locktype_int);
                    __sync_lock_release(&entry_lock_pint[index_int]);
                }
            }
            tmp_src_path_pchar[end_index_int] = '/';
        }
    }
    free_to_NULL(tmp_src_path_pchar);
    return ret_int;
}


static int __serch_and_unset_locktable(
    const char *src_path_pchar,
    const int end_index_int,
    tLockTable *lock_table_pstruct)
{
    int ret_int = ERROR_CODE_SUCCESS;
    //tLockNode **hash_table_ppstruct = lock_table_pstruct->hash_table_pastruct;
    struct list_head **hash_table_ppstruct =
        lock_table_pstruct->hash_table_pastruct;
    uint8_t *entry_lock_pint = lock_table_pstruct->hash_table_entry_lock_aint;

    char *tmp_src_path_pchar = NULL;
    strcpyALL(tmp_src_path_pchar, (char *)src_path_pchar);
    tmp_src_path_pchar[end_index_int] = '\x0';

    int index_int = __get_hash_index(tmp_src_path_pchar);


    eLockType locktype_int;
    if(strlen(src_path_pchar) == end_index_int)
        locktype_int = eWRITE_LOCK;
    else
        locktype_int = eREAD_LOCK;
#ifdef LKM_DEBUG_MODE
    LKM_DEBUG_PRINT("tmp_src_path_pchar = %s, index_int = %d, \
locktype_int = %d\n",
        tmp_src_path_pchar,
        index_int,
        locktype_int );
#endif
    int flag_int = 0;
    while(!flag_int)
    {
        if( !__sync_lock_test_and_set(&entry_lock_pint[index_int], 1))
        {
            flag_int = 1;
            ret_int = __serch_and_unset_tLockNode(
                tmp_src_path_pchar,
                hash_table_ppstruct[index_int],
                locktype_int);
            __sync_lock_release(&entry_lock_pint[index_int]);
        }
    }

    if((ERROR_CODE_SUCCESS == ret_int) &&
        (eREAD_LOCK == locktype_int)) // set success, try to get next lock
    {
        char *nextstart_pchar = &tmp_src_path_pchar[end_index_int+1];
        tmp_src_path_pchar[end_index_int] = '/';
        char *nextstr_pchar = strstr(nextstart_pchar, "/");
        if(NULL != nextstr_pchar)
            *nextstr_pchar = '\x0';
        int next_index_int = strlen(tmp_src_path_pchar);
        ret_int = __serch_and_unset_locktable(
                    src_path_pchar,
                    next_index_int,
                    lock_table_pstruct);
        // unset next lock fail, add this lock
        if(ERROR_CODE_SUCCESS != ret_int)
        {
            tmp_src_path_pchar[end_index_int] = '\x0';
            flag_int = 0;
            while(!flag_int)
            {
                if( !__sync_lock_test_and_set(&entry_lock_pint[index_int], 1))
                {
                    flag_int = 1;
                    __serch_and_set_tLockNode(
                        tmp_src_path_pchar,
                        &hash_table_ppstruct[index_int],
                        locktype_int);
                    __sync_lock_release(&entry_lock_pint[index_int]);
                }
            }
            tmp_src_path_pchar[end_index_int] = '/';
        }
    }
    free_to_NULL(tmp_src_path_pchar);
    return ret_int;
}


int set_table_lock(
    char *src_path_pchar,
    tLockTable *lock_table_pstruct)
{
    int end_index_int = 0;
    char *tmp_src_path_pchar = NULL;
    strcpyALL(tmp_src_path_pchar, src_path_pchar);
    get_first_key(end_index_int, tmp_src_path_pchar);
    //LKM_DEBUG_PRINT("tmp_src_path_pchar = %s, end_index_int = %d\n",
    //    tmp_src_path_pchar,
    //    end_index_int);
    free_to_NULL(tmp_src_path_pchar);

    return __serch_and_set_locktable(
        src_path_pchar,
        end_index_int,
        lock_table_pstruct);
}


int set_table_unlock(
    char *src_path_pchar,
    tLockTable *lock_table_pstruct)
{
    int end_index_int = 0;
    char *tmp_src_path_pchar = NULL;
    strcpyALL(tmp_src_path_pchar, src_path_pchar);
    get_first_key(end_index_int, tmp_src_path_pchar);
    //LKM_DEBUG_PRINT("tmp_src_path_pchar = %s, end_index_int = %d\n",
    //    tmp_src_path_pchar,
    //    end_index_int);
    free_to_NULL(tmp_src_path_pchar);

    return __serch_and_unset_locktable(
        src_path_pchar,
        end_index_int,
        lock_table_pstruct);
}
