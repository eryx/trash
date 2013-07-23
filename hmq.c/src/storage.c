#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#include "config.h"
#include "storage.h"

//#ifdef HAVE_REDIS
#include "storage_redis.c"
//#endif

int storage_connect(int force)
{
  return storage_connect_redis(force);
}

int storage_save(char *q, char *k, char *v)
{
  return storage_save_redis(q, k, v);
}

