#ifndef __lock_mang_H__
#define __lock_mang_H__
#include <stdint.h>
//#include "csg_operation.h"
#include "lock_types.h"

#ifdef CONSOLE_DEBUG
#   define LKM_DEBUG_PRINT(fmt, str...) \
        printf(\
        "%16.16s(%4d) - %16.16s: " fmt, \
        __FILE__, __LINE__, __func__, ##str)
#   define LKM_ERR_PRINT(fmt, str...) \
        printf(\
        "%16.16s(%4d) - %16.16s: **** " fmt, \
        __FILE__, __LINE__, __func__, ##str)
#else
#   include "define.h"
#   define LKM_DEBUG_PRINT PRINT_DBG
#   define LKM_ERR_PRINT PRINT_ERR
#endif
typedef struct _tLockTable tLockTable;

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
    const char *dest_path);

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
    const char *dest_path);

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
    const char *dest_path);

/**
  * Hint :  input data  is no be free in this func
  *
  * Get initail lock table for locking use
  *
  * @return tLockTable*
  *     return create lock table address
  */
tLockTable *initial_lock_table();

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
int destroy_lock_table(tLockTable *table);

/**
  * Hint :  input data  is no be free in this func
  *
  * This method will print the lock table content
  *
  * @param lock_struct type : tLockTable*
  *
  */
void print_lock_table(
    tLockTable *lock_struct);

#endif


