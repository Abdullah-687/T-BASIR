#!/bin/bash


service docker restart &
sleep 0.2
while [ 1 ]; do
	[ `pgrep dockerd` ] && break || continue
done	

/home/user/sscrashanalysis/tracer/src/tracer -p $(pgrep dockerd) >> /home/user/dout.txt &
sleep 20


pkill -SIGINT tracer

