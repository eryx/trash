#ifndef __HASH_H
#define __HASH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define HASH_TABLE_INIT_SIZE 4

typedef struct _HashEntry
{
	char * key;
	void * val;
	struct _HashEntry * next;
} HashEntry;

typedef struct HashType
{
    unsigned int (*hashFunction)(const char *key);
    void *(*keyDup)(void *privdata, const char *key);
    void *(*valDup)(void *privdata, const void *val);
    int (*keyCompare)(void *privdata, const char *key1, const char *key2);
    void (*keyDestructor)(void *privdata, char *key);
    void (*valDestructor)(void *privdata, void *val);
} HashType;

typedef struct _HashTable
{
	int size;
	int sizemask;
	int used;
	HashEntry **table;
	HashType  *type;
} HashTable;

/* API */
static unsigned int hash_gen_hash_function(const unsigned char *buf, int len);
static HashTable * hash_create(HashType *type);

static int hash_resize(HashTable *ht, unsigned long size);
static int hash_insert(HashTable *ht, char *key, void *val);
static int hash_replace(HashTable *ht, char *key, void *val);
static int hash_remove(HashTable *ht, const char *key);
static HashEntry * hash_get(HashTable *ht, const char *key);

static void hash_destroy(HashTable *ht);


#endif
