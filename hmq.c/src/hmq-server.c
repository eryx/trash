#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/queue.h>

#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <err.h>
#include <errno.h>

#include <event.h>
#include <evhttp.h>

#include "../deps/hiredis/hiredis.h"
#include "config.h"
#include "log.h"
#include "storage.h"
#include "worker.h"

int storage_status;

void signal_handler(int sig)
{
  switch (sig) {
    case SIGTERM:
    case SIGHUP:
    case SIGQUIT:
    case SIGINT:
      event_loopbreak();
      fprintf(stderr, "\nSignal(%d) Stop %s [OK]\n\n", sig, HMQ_PRGNAME);
      logger("Signal(%d) Stop %s [OK]", sig, HMQ_PRGNAME);
      break;
  }
}

void request_handler(struct evhttp_request *req, void *arg)
{
  int status = 1;
  struct evbuffer *buf = evbuffer_new();
  if (buf == NULL) return;
  
  struct evkeyvalq hmq_http_query;
  evhttp_parse_query(evhttp_request_uri(req), &hmq_http_query);
  //char *name = (char *)evhttp_find_header(&hmq_http_query, "name");
  char *key  = (char *)evhttp_find_header(&hmq_http_query, "key");
  char *body_get = (char *)evhttp_find_header(&hmq_http_query, "data");
  
  char *uri   = req->uri;//evhttp_request_uri(req);
  char *upath = strtok(uri, "?");
  char *mq    = strtok(upath, "/");
  if (mq == NULL || strcasecmp(mq, "mq")) {
    status = 4; goto loadrsp;
  }
  char *name = strtok(NULL, "/");
  if (name == NULL) {
    status = 4; goto loadrsp;
  }
  char *action = strtok(NULL, "/");
  if (action == NULL || strcasecmp(action, "put")) {
    status = 4; goto loadrsp;
  }
  //printf("req %s/%s/%s\n", mq, mqname, action);

  int len = EVBUFFER_LENGTH(req->input_buffer);  
  if (body_get) {
    status = storage_save(name, key, body_get);
  } else if (len > 0) {
    char *body_put = (char *)malloc(len + 1);
    memset(body_put, '\0', len + 1);
    memcpy(body_put, EVBUFFER_DATA(req->input_buffer), len);
    status = storage_save(name, key, body_put);    
    free(body_put);    
  } else {
    status = 2;
  }
  
  goto loadrsp;

loadrsp:
  evhttp_add_header(req->output_headers, "Server", HMQ_PRGNAME "/" HMQ_VERSION);
  evhttp_add_header(req->output_headers, "Connection", "close"); 
  //evhttp_add_header(req->output_headers, "Keep-Alive", "120"); 
  
  if (status == 0) {
    evbuffer_add_printf(buf, "%s", "200");
    evhttp_send_reply(req, 200, "OK", buf);
  } else if (status == 2) {
    evbuffer_add_printf(buf, "%s", "400");
    evhttp_send_reply(req, 400, "Bad Request", buf);
  } else {
    evbuffer_add_printf(buf, "%s", "503");
    evhttp_send_reply(req, 503, "Service Unavailable", buf);
  }

  evhttp_clear_headers(&hmq_http_query);
  evbuffer_free(buf);
  return;
}

static void help()
{
  fprintf(stderr,
"Hooto Message Queue - %s/%s\n\n"
"@copyright (C) 2011 HOOTO.COM - evorui@gmail.com\n"
"@license   http://www.apache.org/licenses/LICENSE-2.0\n"
"@project   http://github.com/eryx/hmq\n\n"
"Usage: ./hmq-server -c /path/to/hmq-server.conf\n\n"
"  -c <config>    Setting config file\n"
"  -h <help>      Output this help and exit\n"
"  -v <version>   Output version and exit\n"
"\n", HMQ_PRGNAME, HMQ_VERSION);
  exit(1);
}

static void version()
{
  fprintf(stderr, "Hooto Message Queue - %s/%s\n", HMQ_PRGNAME, HMQ_VERSION);
  exit(1);
}

int main(int argc, char **argv)
{
  int opt, err;
  char *config_file = NULL;

  while ((opt = getopt(argc, argv, "c:vh")) != -1) {
    switch (opt) {
      case 'c': config_file = strdup(optarg); break;
      case 'v': version();  break;
      case 'h': help(); break;
      default : break;
    }
  }
  
  //config_file = "../hmq-server.conf";
  if (config_file == NULL) {
    fprintf(stderr, "Fatal error, no config file setting '-c /path/of/hmq-server.conf'\n\n");
    exit(1);
  }

  initConfig();
  loadConfig(config_file);
  
  if (cfg.daemon == true) {
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);
  }
  
  logger("Start %s/%s [OK]", HMQ_PRGNAME, HMQ_VERSION);
  
  //
  signal(SIGHUP,  signal_handler);
  signal(SIGTERM, signal_handler);
  signal(SIGINT,  signal_handler);
  signal(SIGQUIT, signal_handler);
  //signal(SIGPIPE, SIG_IGN);
 
  // Start Worker
  err = pthread_create(&worker_ptid, NULL, worker_start, NULL);
  if (err != 0) {
    fprintf(stderr, "Error: Can not create worker thread '%s'\n", strerror(err));
    exit(1);
  }
  
  //
  FILE *fp_pidfile = fopen(cfg.pidfile, "w");
  if (fp_pidfile == NULL) {
    fprintf(stderr, "Error: Can not create pidfile '%s'\n\n", cfg.pidfile);
    exit(1);
  }
  fprintf(fp_pidfile, "%d\n", getpid());
  fclose(fp_pidfile);
  

  struct evhttp *httpd;
  event_init();
  httpd = evhttp_start("0.0.0.0", cfg.port);
  if (httpd == NULL) {
    fprintf(stderr, "Error: Unable to listen on 0.0.0.0:%d\n\n", cfg.port);
    exit(1);
  }
  evhttp_set_timeout(httpd, cfg.http_timeout);
  
  storage_status = storage_connect(1);
  if (storage_status == STORAGE_ERR) {
    fprintf(stderr, "Error: Unable to connect on Storage\n");
    exit(1);
  }
  
  /* Set a callback for requests to "/specific". */
  /* evhttp_set_cb(httpd, "/select", select_handler, NULL); */

  /* Set a callback for all other requests. */
  evhttp_set_gencb(httpd, request_handler, NULL);

  event_dispatch();

  /* Not reached in this code as it is now. */
  evhttp_free(httpd);

  return 0;
}
