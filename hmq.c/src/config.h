#ifndef __CONFIG_H
#define __CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <paths.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <err.h>
#include <time.h>
#include <ctype.h>
#include <math.h>

#include "worker.h"

#define HMQ_PRGNAME         "hmq"
#define HMQ_VERSION         "1.0.0"
#define HMQ_CONFIGLINE_MAX  1024

pthread_t worker_ptid;

struct configObject
{
  int     port;
  int     daemon;
  char   *pidfile;
  int     http_timeout;
  
  char   *storage_redis_host;
  int     storage_redis_port;
  
  struct _WorkEntry *worker;
  
  char   *logfile;
  time_t  _log_reopen;
  int     _log_fd;
} cfg;

void initConfig();
void loadConfig(char *filename);

#endif
