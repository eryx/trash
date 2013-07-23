#ifndef __WORKER_H
#define __WORKER_H

#include <unistd.h>
#define MAX_BUFFER_SIZE 1024
#define MAX_WORKER_SIZE 4

#ifndef STDIN_FILENO
#define STDIN_FILENO    0
#endif
#ifndef STDOUT_FILENO
#define STDOUT_FILENO   1
#endif
#ifndef STDERR_FILENO
#define STDERR_FILENO   2
#endif

typedef struct _WorkEntry
{
  pid_t   pid;
  int     p[2];
  char    *key;
  int     status;
  char    *cmd;
  struct _WorkEntry *prior;
  struct _WorkEntry *next;
} WorkEntry;

void * worker_start(void *arg);

#endif
