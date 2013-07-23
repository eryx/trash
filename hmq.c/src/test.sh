#!/bin/sh
/etc/init.d/hmq-server stop
make clean
make
make install
./hmq-server -c ../hmq-server.conf
#/etc/init.d/hmq-server start

#ab -c 100 -n 100000 "http://localhost:9528/mq/h1/put/?data=TEST-ONE"
