#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <memory.h>
#include <assert.h>

#include "hash.c"
#include "sds.h"



typedef struct queue {
    struct queue *next;
    void *privdata;
} queue;


static unsigned int queueHash(const void *key) {
    return hash_gen_hash_function((unsigned char*)key,sdslen((char*)key));
}

static void *queueValDup(void *privdata, const void *src) {
    ((void) privdata);
    queue *dup = malloc(sizeof(*dup));
    memcpy(dup,src,sizeof(*dup));
    return dup;
}

static int queueKeyCompare(void *privdata, const void *key1, const void *key2) {
    int l1, l2;
    ((void) privdata);

    l1 = sdslen((sds)key1);
    l2 = sdslen((sds)key2);
    if (l1 != l2) return 0;
    return memcmp(key1,key2,l1) == 0;
}

static void queueKeyDestructor(void *privdata, void *key) {
    ((void) privdata);
    sdsfree((sds)key);
}

static void queueValDestructor(void *privdata, void *val) {
    ((void) privdata);
    free(val);
}

static HashType mq = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

int main(void)
{
	HashTable *ht = hash_create(&mq);

  int i, ok = 0, er = 0, tl = 10;
  for (i = 0; i < tl; i++) {

    char key[10];
    char val[50];

    sprintf(key, "key%d", i);
    sprintf(val, "val%d", i);

    if (hash_replace(ht, key, val) != 0) er++;
    else ok++;
  }
  //sleep(5);
  //printf("status ok:%d er:%d, size:%d, used:%d\n", ok, er, ht->size, ht->used);
  
  // LIST ALL
  for (i = 0; i < tl; i++) {
    char key[10];
    sprintf(key, "key%d", i);
    HashEntry *entry = hash_get(ht, key);
    if (entry) {
      printf("\t[%d/%d] [%s/%s]\n", i, ht->size, entry->key, entry->val);
    }
  }
  
  // DES
  hash_destroy(ht);
  ht = NULL;
    
  printf("OK\n");
	sleep(10);
	return 0;
}
