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
    int strupper_index = 0;\
    while(buf[strupper_index])\
    {\
        if(buf[strupper_index] > 'a' && buf[strupper_index] < 'z')\
            buf[strupper_index++] &= '_';\
        else\
            strupper_index++;\
    }\
\
}while(0)

/**
  * Make english uppercase letters to lowercase
  * e.g. AAA.cc@BB.cOm to aaa.cc@bb.com
  *
  */
#define strlower(buf) do{\
    int strlower_index = 0;\
    while(buf[strlower_index])\
    {\
        if(buf[strlower_index] > 'A' && buf[strlower_index] < 'Z')\
            buf[strlower_index++] |= ' ';\
        else\
            strlower_index++;\
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
    int strcpyALL_len = 1, strcpyALL_index = 0;\
    while(a[strcpyALL_index])\
    {\
        strcpyALL_len += strlen(a[strcpyALL_index++]);\
    }\
    buf = malloc(sizeof(char[strcpyALL_len]));\
    memset(buf, 0, sizeof(char[strcpyALL_len]));\
    strcpyALL_index = 0;\
    while(a[strcpyALL_index])\
    {\
        strcat((char *)buf, a[strcpyALL_index++]);\
    }\
}while(0)

#define RAII_VARIABLE(vartype, varname, initval, dtor) \
    void _dtor_ ## varname (vartype * v) { dtor(*v); } \
    vartype varname __attribute__((cleanup(_dtor_ ## varname))) = (initval)

#define free_to_NULL(ptr) do{\
    if(NULL != (ptr) ) {free(ptr);ptr = NULL;}\
}while(0)


#endif
