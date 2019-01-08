/*------------------------------------------------------------------------------------------------*/
/* Utility support functions */
/* Begun 2018-2019, StackHPC Ltd */

#ifndef __UTILS_H__                                              /* __UTILS_H__ */
#define __UTILS_H__                                              /* __UTILS_H__ */

#include <stdio.h>
#include <stdarg.h>

/* Return the number of members in a statically-defined array */
#define ARRAYLEN(x)     (sizeof(x) / sizeof(*x))

/*------------------------------------------------------------------------------------------------*/
/* Logging functions:
 * Copyright (c) 2017 rxi (Adapted and extended by StackHPC, 2019)
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `log.c` for details.
 */

#define LOG_VERSION "0.1.0"

typedef void (*log_LockFn)(void *udata, int lock);

enum { LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL };

#define log_trace(...) log_log(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) log_log(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...)  log_log(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...)  log_log(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) log_log(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_fatal(...) log_log(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

void log_set_udata(void *udata);
void log_set_lock(log_LockFn fn);
void log_set_fp(FILE *fp);
void log_set_level(int level);
void log_set_quiet(int enable);

void log_log(int level, const char *file, int line, const char *fmt, ...);

#endif                                                          /* __UTILS_H__ */
