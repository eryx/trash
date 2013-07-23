#include <err.h>
#include <errno.h>
#include "config.h"
#include "storage.h"
#include "../deps/hiredis/hiredis.h"

static redisContext *redis_ctt;

int storage_connect_redis(int force)
{
  //printf("storage_connect_redis %d\n", force);
  
  if (redis_ctt == NULL || force) {
  
    if (redis_ctt != NULL)
      redisFree(redis_ctt);
    
    redis_ctt = redisConnect(cfg.storage_redis_host, cfg.storage_redis_port);
    if (redis_ctt->err) {
      redisFree(redis_ctt);
      redis_ctt = NULL;
      return STORAGE_ERR;
    }
  }
  
  return STORAGE_OK;
}

int storage_close_redis()
{
  return STORAGE_OK;
}

int storage_save_redis(char *q, char *k, char *v)
{
  if (q == NULL || v == NULL)
    return STORAGE_ERR;

  if (redis_ctt == NULL)
    storage_connect_redis(0);
  
  if ((redis_ctt->err == REDIS_ERR_IO && errno == ECONNRESET)
    || redis_ctt->err == REDIS_ERR_EOF) {
    storage_connect_redis(1);
  }
  if ((redis_ctt->err == REDIS_ERR_IO && errno == ECONNRESET)
    || redis_ctt->err == REDIS_ERR_EOF) {
    return STORAGE_ERR;
  }

  redisReply *reply = redisCommand(redis_ctt, "LPUSH %s %b", q, v, strlen(v));
  if (reply == NULL) {
    return STORAGE_ERR;
  }
  
  freeReplyObject(reply);
  return STORAGE_OK;
}
