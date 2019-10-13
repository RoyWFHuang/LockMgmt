#ifndef __macro_H__
#define __macro_H__

#include <stdlib.h>
#include <string.h>
#include "errorno.h"

#define check_null_input(ptr) do{\
    if(NULL == ptr) return ERROR_CODE_NULL_POINT_EXCEPTION;\
}while(0)

/**
  * Make english lowercase letters to uppercase
  * e.g. aaa.CC@bb.CoM to AAA.CC@BB.COM
  *
  */
#define strupper(buf) do{\
    int strupper_index_int = 0;\
    while(buf[strupper_index_int])\
    {\
        if(buf[strupper_index_int] > 'a' && buf[strupper_index_int] < 'z')\
            buf[strupper_index_int++] &= '_';\
        else\
            strupper_index_int++;\
    }\
\
}while(0)

/**
  * Make english uppercase letters to lowercase
  * e.g. AAA.cc@BB.cOm to aaa.cc@bb.com
  *
  */
#define strlower(buf) do{\
    int strlower_index_int = 0;\
    while(buf[strlower_index_int])\
    {\
        if(buf[strlower_index_int] > 'A' && buf[strlower_index_int] < 'Z')\
            buf[strlower_index_int++] |= ' ';\
        else\
            strlower_index_int++;\
    }\
\
}while(0)

/**
  * cat multi-string to one string,
  * e.g. str1 = "aaa", str2 = "bbb", str3 = "c4cd"
  * return string is aaabbbc4cd
  *
  */
#define strcpyALL(buf,...) do{ \
    char *a[] = { __VA_ARGS__, NULL}; \
    int strcpyALL_len_int = 1, strcpyALL_index_int = 0;\
    while(a[strcpyALL_index_int])\
    {\
        strcpyALL_len_int += strlen(a[strcpyALL_index_int++]);\
    }\
    buf = malloc(sizeof(char[strcpyALL_len_int]));\
    memset(buf, 0, sizeof(char[strcpyALL_len_int]));\
    strcpyALL_index_int = 0;\
    while(a[strcpyALL_index_int])\
    {\
        strcat((char *)buf, a[strcpyALL_index_int++]);\
    }\
}while(0)

#define free_to_NULL(ptr) do{\
    if(NULL != (ptr) ) {free(ptr);ptr = NULL;}\
}while(0)


#endif
