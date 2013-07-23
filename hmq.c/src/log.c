#include "config.h"
#include "log.h"

void logger(const char *s, ...)
{
  int fd;
  char msg[HMQ_LOG_MAX_ERROR_SIZE], vmsg[HMQ_LOG_MAX_ERROR_SIZE];
  time_t t = time(NULL);

  if (cfg.logfile == NULL)
    fd = 1;
  else
  {
    if (cfg._log_fd != -1 && cfg._log_reopen < t) {
      (void) close(cfg._log_fd);
      cfg._log_fd = -1;
    }

    if (cfg._log_fd == -1) {
      cfg._log_fd = open(cfg.logfile, O_CREAT | O_WRONLY | O_APPEND, 0644);
      if (cfg._log_fd == -1) {
        fprintf(stderr, "hmq: Could not open log file for writing (%d)\n",
                errno);
        return;
      }

      cfg._log_reopen = t + HMQ_LOG_REOPEN_TIME;
    }

    fd = cfg._log_fd;
  }
  
  va_list ap;
  va_start(ap, s);
  vsnprintf(vmsg, HMQ_LOG_MAX_ERROR_SIZE, s, ap);
  va_end(ap);
  
  struct tm *ptm = localtime(&t);
  char sftime[50];
  strftime(sftime, 50, "%F %T %Z", ptm);
  
  snprintf(msg, HMQ_LOG_MAX_ERROR_SIZE, "[%s] %s\n", sftime, vmsg);
  if (write(fd, msg, strlen(msg)) == -1)
    fprintf(stderr, "hmq: Could not write to log file: %d\n", errno);
}
