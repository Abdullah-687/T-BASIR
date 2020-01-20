#!/bin/bash

/usr/local/cassandra/bin/cassandra &

while [ 1 ]; do
	[ `pgrep java` ] && break || continue
done	

sudo /home/user/sscrashanalysis/tracer/src/tracer -p $(pgrep java) >> /home/user/cout.txt &
sleep 20

 
sudo pkill -9 java
sudo pkill -SIGINT tracer

