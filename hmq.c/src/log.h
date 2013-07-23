#ifndef __LOG_H
#define __LOG_H

#include <stdio.h>
#include <stdarg.h>
#include <time.h>   
#include <sys/time.h>
#include <fcntl.h>
#include <err.h>
#include <errno.h>


#define HMQ_LOG_REOPEN_TIME     60
#define HMQ_LOG_MAX_ERROR_SIZE  1024

void logger(const char *s, ...);

#endif
