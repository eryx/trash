#include "config.h"
#include "worker.h"
#include "../deps/hiredis/sds.h"

void initConfig()
{
  cfg.port    = 9528;
  cfg.daemon  = 0;
  cfg.pidfile = "/tmp/hmq.pid";
  cfg.http_timeout = 3;
  
  cfg.storage_redis_host  = "127.0.0.1";
  cfg.storage_redis_port  = 6379;  
  
  cfg.worker      = NULL;
  
  cfg.logfile     = NULL;
  cfg._log_reopen = 0;
  cfg._log_fd     = -1;
}

void loadConfig(char *filename) 
{
  FILE *fp;
  char buf[HMQ_CONFIGLINE_MAX+1];
  int linenum = 0, i;
  sds line = NULL;

  if (filename[0] == '-' && filename[1] == '\0')
    fp = stdin;
  else {
    if ((fp = fopen(filename, "r")) == NULL) {
      fprintf(stderr, "Fatal error, can't open config file '%s'\n\n", filename);
      exit(1);
    }
  }
  
  while (fgets(buf, HMQ_CONFIGLINE_MAX+1, fp) != NULL)
  {
    sds *argv;
    int argc;

    linenum++;
    line = sdsnew(buf);
    line = sdstrim(line, " \t\r\n");

    /* Skip comments and blank lines*/
    if (line[0] == '#' || line[0] == '\0') {
      sdsfree(line);
      continue;
    }

    /* Split into arguments */
    argv = sdssplitargs(line, &argc);
    sdstolower(argv[0]);
    
    /* Execute config directives */
    if (!strcasecmp(argv[0], "http_timeout") && argc == 2) {
      cfg.http_timeout = atoi(argv[1]);
    } else if (!strcasecmp(argv[0], "port") && argc == 2) {
      cfg.port = atoi(argv[1]);
    } else if (!strcasecmp(argv[0], "daemonize") && argc == 2) {
      if (!strcasecmp(argv[1], "yes")) cfg.daemon = 1;
    } else if (!strcasecmp(argv[0], "pidfile") && argc == 2) {
      cfg.pidfile = strdup(argv[1]);
    } else if (!strcasecmp(argv[0], "storage_redis_host") && argc == 2) {
      cfg.storage_redis_host = strdup(argv[1]);
    } else if (!strcasecmp(argv[0], "storage_redis_port") && argc == 2) {
      cfg.storage_redis_port = atoi(argv[1]);
    } else if (!strcasecmp(argv[0], "worker") && argc > 1) {
    
      char cmd[HMQ_CONFIGLINE_MAX];
      char *to = cmd;
      for (i = 1; i < argc; i++) {
        to = stpcpy(to, argv[i]);
        to = stpcpy(to, " ");
      }

      WorkEntry *entry = malloc(sizeof(WorkEntry));
      entry->key    = "0";
      entry->cmd    = strdup(cmd);
      entry->status = 0;
      entry->pid    = 0;
      entry->prior  = NULL;
      entry->next   = NULL;
      
      if (cfg.worker != NULL) {
        cfg.worker->prior = entry;
        entry->next       = cfg.worker;
      }
      cfg.worker = entry;
      //printf("debug: cfg.worker: insert %s\n", entry->key);
    } else if (!strcasecmp(argv[0], "logfile") && argc == 2) {
      cfg.logfile     = strdup(argv[1]);
      cfg._log_fd     = -1;
      cfg._log_reopen = 0;
    }
  }
}

