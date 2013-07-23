#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include "hash.h"

/* -------------------------- private prototypes ---------------------------- */
static int  _hash_resize_if_needed(HashTable *ht);
static int  _hash_key_index(HashTable *ht, const char *key);
static void _hash_reset(HashTable *ht);

/* ----------------------------- API implementation ------------------------- */

/* Reset an hashtable already initialized with ht_init().
 * NOTE: This function should only called by ht_destroy(). */
static void _hash_reset(HashTable *ht)
{
    ht->table = NULL;
    ht->size = 0;
    ht->sizemask = 0;
    ht->used = 0;
}

/* Generic hash function (a popular one from Bernstein).
 * I tested a few and this was the best. */
static unsigned int hash_gen_hash_function(const unsigned char *buf, int len)
{
    unsigned int hash = 5381;
    while (len--)
        hash = ((hash << 5) + hash) + (*buf++); /* hash * 33 + c */
    return hash;  
}

/* Create a new hash table */
static HashTable * hash_create(HashType *type)
{
    HashTable *ht = malloc(sizeof(*ht));
    _hash_reset(ht);
    ht->type = type;
    return ht;
}

/* Expand or create the hashtable */
static int hash_resize(HashTable *ht, unsigned long size)
{
    printf("DEBUG: hash_resize %d -> %d\n", ht->size, size);
    
    HashTable n; /* the new hashtable */
    int i;

    /* the size is invalid if it is smaller than the number of
     * elements already inside the hashtable */
    if (ht->used > size || size >= LONG_MAX) return 1;

    _hash_reset(&n);
    n.size      = size;
    n.sizemask  = size-1;
    n.table     = calloc(size, sizeof(HashEntry *));

    /* Copy all the elements from the old to the new table:
     * note that if the old hash table is empty ht->size is zero,
     * so hashExpand just creates an hash table. */
    n.used = ht->used;
    for (i = 0; i < ht->size && ht->used > 0; i++) {
        HashEntry *he, *nextHe;

        if (ht->table[i] == NULL) continue;

        /* For each hash entry on this slot... */
        he = ht->table[i];
        while (he) {
            unsigned int h;

            nextHe = he->next;
            /* Get the new element index */
            h = hash_gen_hash_function(he->key, strlen(he->key)) & n.sizemask;
            he->next = n.table[h];
            n.table[h] = he;

            ht->used--;
            /* Pass to the next element */
            he = nextHe;
        }
    }
    //assert(ht->used == 0);
    free(ht->table);
        
    /* Remap the new hashtable in the old */
    *ht = n;
    return 0;
}


/* Add an element to the target hash table */
static int hash_insert(HashTable *ht, char *key, void *val)
{
    int index = _hash_key_index(ht, key);

    /* Get the index of the new element, or -1 if
     * the element already exists. */
    if (index == -1) return 1;
    
    /* Allocates the memory and stores key */
    HashEntry *entry = malloc(sizeof(*entry));
    entry->next = ht->table[index];
    entry->key  = strdup(key);
    entry->val  = strdup(val);
    
    ht->table[index] = entry;
    ht->used++;
    
    //printf("DEBUG INSERT [%d/%d] [%s/%s]\n", index, ht->size, key, val);
    return 0;
}

/* Insert/Replace an element to the target hash table */
static int hash_replace(HashTable *ht, char *key, void *val)
{
    /* Try to add the element. If the key
     * does not exists dictAdd will suceed. */
    if (hash_insert(ht, key, val) == 0)
        return 0;
    
    /* It already exists, get the entry */
    HashEntry *entry = hash_get(ht, key);
    /* Free the old value and set the new one */
    /* Set the new value and free the old one. Note that it is important
     * to do that in this order, as the value may just be exactly the same
     * as the previous one. In this context, think to reference counting,
     * you want to increment (set), and then decrement (free), and not the
     * reverse. */
    free(entry->val);
    entry->val = strdup(val);

    //printf("DEBUG REPLACE [%d/%d] [%s/%s]\n", index, ht->size, key, val);
    return 0;
}

static HashEntry *hash_get(HashTable *ht, const char *key) 
{
    HashEntry *he;
    unsigned int h;

    if (ht->size == 0) return NULL;
    h = hash_gen_hash_function(key, strlen(key)) & ht->sizemask;
    he = ht->table[h];
    while (he) {
        if (strcmp(key, he->key) == 0)
            return he;
        he = he->next;
    }
    return NULL;
}


/* Search and remove an element */
static int hash_remove(HashTable *ht, const char *key) 
{
    unsigned int h;
    HashEntry *de, *prevde;

    if (ht->size == 0)
        return 1;
    h = hash_gen_hash_function(key, strlen(key)) & ht->sizemask;
    de = ht->table[h];

    prevde = NULL;
    while(de) {
        if (strcmp(key, de->key) == 0) {
            /* Unlink the element from the list */
            if (prevde)
                prevde->next = de->next;
            else
                ht->table[h] = de->next;

            free(de->key);
            free(de->val);
            free(de);
            
            ht->used--;
            //printf("DEBUG REMOVE [%d/%d] [%s]\n", h, ht->size, key);
            return 0;
        }
        prevde = de;
        de = de->next;
    }
    return 1; /* not found */
}

/* Destroy an entire hash table */
static void hash_destroy(HashTable *ht)
{
    unsigned long i;

    /* Free all the elements */
    for (i = 0; i < ht->size && ht->used > 0; i++) {
        HashEntry *he, *nextHe;

        if ((he = ht->table[i]) == NULL) continue;
        while(he) {
            nextHe = he->next;
            free(he->key);
            free(he->val);
            free(he);
            ht->used--;
            he = nextHe;
        }
    }

    free(ht->table);
    free(ht);
}


/* ------------------------- private functions ------------------------------ */

/* Resize the hash table if needed */
static int _hash_resize_if_needed(HashTable  *ht)
{
    if (ht->size == 0)
        return hash_resize(ht, HASH_TABLE_INIT_SIZE);
    if (ht->used >= ht->size)
        return hash_resize(ht, ht->size*2);
    return 0;
}

/* Returns the index of a free slot that can be populated with
 * an hash entry for the given 'key'.
 * If the key already exists, -1 is returned. */
static int _hash_key_index(HashTable  *ht, const char *key)
{
    unsigned int h;
    HashEntry *he;

    /* Expand the hashtable if needed */
    if (_hash_resize_if_needed(ht) == 1)
        return -1;

    /* Compute the key hash value */
    h = hash_gen_hash_function(key, strlen(key)) & ht->sizemask;
    /* Search if this slot does not already contain the given key */
    he = ht->table[h];
    while (he) {
        if (strcmp(key, he->key) == 0) {
            //printf("ERROR %s %d\n", __FILE__, __LINE__);
            return -1;
        }
        he = he->next;
    }
    return h;
}

