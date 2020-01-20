#!/bin/bash

service postgresql stop
service postgresql start &

while [ 1 ]; do
	[ `pgrep postgres | head -1` ] && break || continue
done	

/home/user/sscrashanalysis/tracer/src/tracer -p $(pgrep postgres | head -1) >> /home/user/postgres_results/out.txt &
sleep 20


service postgresql stop &
pkill -SIGINT tracer

