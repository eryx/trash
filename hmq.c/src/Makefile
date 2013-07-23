
CC=gcc

#CFLAGS=-I. -I./include -levent -L../deps/hiredis/ -lhiredis -I../deps/hiredis -L../deps/leveldb -lleveldb -I../deps/leveldb/include
CFLAGS=-I../deps/hiredis -L../deps/hiredis -lhiredis -levent  -lpthread -g

uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
OPTIMIZATION?=-O3

CCCOLOR="\033[34m"
LINKCOLOR="\033[34;1m"
SRCCOLOR="\033[33m"
BINCOLOR="\033[37;1m"
MAKECOLOR="\033[32;1m"
ENDCOLOR="\033[0m"

ifndef V
QUIET_CC = @printf '    %b %b\n' $(CCCOLOR)CC$(ENDCOLOR) $(SRCCOLOR)$@$(ENDCOLOR);
QUIET_LINK = @printf '    %b %b\n' $(LINKCOLOR)LINK$(ENDCOLOR) $(BINCOLOR)$@$(ENDCOLOR);
endif

ifeq ($(uname_S),SunOS)
  CFLAGS?= -std=c99 -pedantic $(OPTIMIZATION) -Wall -W -D__EXTENSIONS__ -D_XPG6
  CCLINK?= -ldl -lnsl -lsocket -lm -lpthread
  DEBUG?= -g -ggdb 
else
  CFLAGS?= -std=c99 -pedantic $(OPTIMIZATION) -Wall -W $(ARCH) $(PROF)
  CCLINK?= -lm -pthread
  DEBUG?= -g -rdynamic -ggdb 
endif


ifeq ($(USE_TCMALLOC),yes)
  CCLINK+= -ltcmalloc
  CFLAGS+= -DUSE_TCMALLOC
endif
CCOPT= $(CFLAGS) $(ARCH) $(PROF)

PREFIX= /opt/hmq
INSTALL_BIN= $(PREFIX)/bin
INSTALL= cp -p


OBJ = hmq-server.o config.o storage.o log.o worker.o ../deps/hiredis/libhiredis.a

PRGNAME = hmq-server
HASHTESTNAME = hash_test

all: dependencies hmq-server
	@echo ""
	@echo "hmq* build complete ;)"
	@echo ""

# Deps (use make dep to generate this)
hmq-server.o: hmq-server.c
config.o:     config.c
storage.o:    storage.c
worker.o:     worker.c
log.o:        log.c
hash.o:       hash.c

dependencies:
	@printf '%b %b\n' $(MAKECOLOR)MAKE$(ENDCOLOR) $(BINCOLOR)hiredis$(ENDCOLOR)
	cd ../deps/hiredis && $(MAKE) static ARCH="$(ARCH)"

hmq-server: dependencies $(OBJ)
	@printf '%b %b\n' $(MAKECOLOR)MAKE$(ENDCOLOR) $(BINCOLOR)hiredis$(ENDCOLOR)
	cd ../deps/hiredis && $(MAKE) static ARCH="$(ARCH)"
	$(QUIET_LINK)$(CC) -o $(PRGNAME) $(CCOPT) $(DEBUG) $(OBJ) $(CCLINK)

hash_test:
	$(QUIET_LINK)$(CC) -o $(HASHTESTNAME) $(CCOPT) $(DEBUG) hash_test.c ../deps/hiredis/libhiredis.a $(CCLINK)

clean:
	rm -rf $(PRGNAME) *.out *.o *.gcda *.gcno *.gcov

dep:
	$(CC) -MM *.c -I ../deps/hiredis 


32bit:
	@echo ""
	@echo "WARNING: if it fails under Linux you probably need to install libc6-dev-i386"
	@echo ""
	$(MAKE) ARCH="-m32"

install: all
	mkdir -p $(INSTALL_BIN)
	$(INSTALL) $(PRGNAME) $(INSTALL_BIN)
	

