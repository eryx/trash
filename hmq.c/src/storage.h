#ifndef __STORAGE_H__
#define __STORAGE_H__

#define STORAGE_OK    0
#define STORAGE_ERR   -1


/* Prototypes */
int storage_connect(int force);
int storage_close();
int storage_save(char *q, char *k, char *v);

#endif
