#!/bin/bash

service mongod stop
service mongod start &


while [ 1 ]; do
	[ `pgrep mongod | head -1` ] && break || continue
done	

/home/user/sscrashanalysis/tracer/src/tracer -p $(pgrep mongod | head -1) >> /home/user/mongodb_results/out.txt &
sleep 20


service mongod stop &
pkill -SIGINT tracer

