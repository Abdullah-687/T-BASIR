#!/bin/bash

/home/user/sscrashanalysis/tracer/src/tracer /usr/sbin/mysqld &
sleep 10
pkill -9 mysqld
pkill -SIGINT tracer



