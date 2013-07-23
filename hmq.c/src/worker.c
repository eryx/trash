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

#include "config.h"
#include "worker.h"
#include "log.h"

void * worker_start(void *arg)
{
  int devnull, wstatus, i;
  pid_t wait_pid;
  int running = 1, signaled = 0, stopped = 0;
	int exitcode = -1, termsig = 0, stopsig = 0;
	
  while (1) {
    
    WorkEntry *entry = cfg.worker, *entry_next;
    
    while (entry) {
    
      //logger("checking key:%s", entry->key);
      
      // start/restart worker
      if (entry->status == 0) {
    
        if ((devnull = open("/dev/null", O_RDWR)) == -1) {
          logger("\tCouldn't open /dev/null %d", __LINE__);
          entry = entry->next; continue;
        }    
        if (pipe(entry->p) == -1) {
          logger("\tCouldn't open pipe %d", __LINE__);
          entry = entry->next; continue;
        }
        if ((entry->pid = fork()) == -1) {
          logger("\tCouldn't fork %d", __LINE__);
          entry = entry->next; continue;
        }
    
        if (entry->pid == 0) {
    
          dup2(devnull, STDIN_FILENO);
          dup2(entry->p[1], STDOUT_FILENO);

          /**sds *argv;
          int argc;
          argv = sdssplitargs(entry->cmd, &argc);
          fprintf(stderr, "\targc: %d, cmd:%s\n", argc, argv[0]);

          char *argv2[argc+1];
          argv2[0] = "sh";
          for (i = 1; i < argc; i++) {
            argv2[i] = strdup(argv[i]);
          }
          argv2[argc] = NULL;
          execv(argv[0], argv2);
          */
          
          execl(entry->cmd, "sh", NULL);
          //execl("ps -x", "ps", NULL);
          printf(stderr, "\t(ps child) Couldn't exec '%s'", entry->cmd);
          _exit(1);
        }
    
        close(devnull);
        close(entry->p[1]);

        /*char buf[200];
        int ret;
        memset(buf, '\0', sizeof(buf));
        while (ret = read(entry->p[0], buf, sizeof(buf))) {
          if (ret == -1)
            printf("Couldn't read from ps\n");
          else
            printf("child proc: %s\n", buf);
        }*/
    
        entry->status = 1;
        logger("worker start/restart, key:%s, cmd:%s", entry->key, entry->cmd);
        entry = entry->next; continue;
      }
      
      // checking
      if (entry->pid == -1) {
        logger("worker waiting, key:%s", entry->key);
        entry = entry->next; continue;
      }
    
      running = 1;
      exitcode = -1;
      wait_pid = waitpid(entry->pid, &wstatus, WNOHANG|WUNTRACED);
      //wait_pid = waitpid(entry->pid, &wstatus, 0);
	    printf("\tw status %d\n", wstatus);
	    if (wait_pid == entry->pid) {
		    if (WIFEXITED(wstatus)) {
			    running = 0;
			    exitcode = WEXITSTATUS(wstatus);
			    printf("Child exited with code %d\n", WEXITSTATUS(wstatus));
		    }
		    if (WIFSIGNALED(wstatus)) {
			    running = 0;
			    signaled = 1;
    			termsig = WTERMSIG(wstatus);
    		}
		    if (WIFSTOPPED(wstatus)) {
		    	stopped = 1;
		    	stopsig = WSTOPSIG(wstatus);
		    	printf("Child exited with WIFSTOPPED\n");
		    }
	    } else if (wait_pid == -1) {
		    running = 0;
		    printf("Couldn't wait for ps completion\n");
	    }
	    
	    if (running) {
	      logger("worker running, key:%s, cmd:%s, code:%d", entry->key, entry->cmd, exitcode);
	      //entry->status = 1;
      } else {
        logger("worker stopped, key:%s, cmd:%s, code:%d", entry->key, entry->cmd, exitcode);
        entry->pid    = 0;
        entry->status = 0;
      }
      
      entry = entry->next;
    }
    
    sleep(1);
  }
  
  return;
}
