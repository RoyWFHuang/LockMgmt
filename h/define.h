#ifndef __define_H__
#define __define_H__
#include <sys/syslog.h>
#include <pthread.h>
#ifdef __GNUC__
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif


#ifdef ERROR_MSG_MODE
#   ifdef SYSLOG
#       define PRINTERR_OUTFILE stderr
#   endif /* PRINTERR_OUTFILE */
#   ifndef PRINT_ERR
#       define PRINT_ERR log_err
#   endif /* PRINT_ERR */
#else
#   ifndef PRINT_ERR
#       define PRINT_ERR(fmt, str...)
#   endif /* PRINT_ERR */
#endif /* ERROR_MSG_MODE */

#ifdef DEBUG_MODE
#   ifndef SYSLOG
#       define PRINTMSG_OUTFILE stdout
#   endif /* PRINTMSG_OUTFILE */
#   ifndef PRINT_DBG
#       define PRINT_DBG log_debug
#   endif /* PRINT_DBG */
#else
#   ifndef PRINT_DBG
#       define PRINT_DBG(fmt, str...)
#   endif /* PRINT_DBG */
#endif /* DEBUG_MODE */

#ifndef SYSLOG
#   define debuglog(level, mode, fmt, ...) \
    fprintf(level, "[%lu] [%5.5s] - %16.16s(%4d) - %16.16s: " fmt , \
        pthread_self(), mode, __FILE__, __LINE__, __func__, ##__VA_ARGS__);
#define log_debug(fmt, ...) \
    debuglog(PRINTMSG_OUTFILE, "DEBUG", fmt, ##__VA_ARGS__);
#define log_err(fmt, ...) \
    debuglog(PRINTERR_OUTFILE, "ERROR", fmt, ##__VA_ARGS__);
#else
#   define debuglog(level, mode, fmt, ...) \
    syslog(level, "[%lu] [%5.5s] - %16.16s(%4d) - %16.16s: " fmt , \
        pthread_self(), mode, __FILE__, __LINE__, __func__, ##__VA_ARGS__);
#define log_debug(fmt, ...) \
    debuglog(LOG_DEBUG, "DEBUG", fmt, ##__VA_ARGS__);
#define log_err(fmt, ...) \
    debuglog(LOG_ERR, "ERROR", fmt, ##__VA_ARGS__);
#endif



#endif