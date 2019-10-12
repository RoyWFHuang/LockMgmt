#ifndef __lock_keynode_H__
#define __lock_keynode_H__


#include <stdint.h>
#include "list.h"
#ifndef MAX_LOCK_TABLE_POWER_OF_2
#   define MAX_LOCK_TABLE_POWER_OF_2 (16)
#endif

#define MAX_LOCK_TABLE_INDEX (1 << MAX_LOCK_TABLE_POWER_OF_2)





typedef enum _eLockType
{
    eREAD_LOCK,
    eWRITE_LOCK,
}eLockType;

typedef struct _tLockNode
{
    uint8_t node_entry_lock_int;
    char *name_pchar;
    uint8_t write_lock;
    uint64_t read_lock;
    struct list_head list_struct;
}tLockNode;


struct _tLockTable
{
    uint8_t destroy_lock_int;
    uint64_t used_cnt_int;
    struct list_head *hash_table_pastruct[MAX_LOCK_TABLE_INDEX];
    uint8_t hash_table_entry_lock_aint[MAX_LOCK_TABLE_INDEX];
};

int set_table_lock(
    char *src_path_pchar,
    struct _tLockTable *lock_table_pstruct);

int set_table_unlock(
    char *src_path_pchar,
    struct _tLockTable *lock_table_pstruct);


#endif