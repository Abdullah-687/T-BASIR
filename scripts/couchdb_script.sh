#!/bin/bash



for i in {1..30}
do
	service couchdb stop
	service couchdb start &


	while [ 1 ]; do
		[ `pgrep beam | head -1` ] && break || continue
	done	

	/home/user/sscrashanalysis/tracer/src/tracer -p $(pgrep beam | head -1) >> /home/user/couchdb_results/output_couchdb_$1_$i.txt &
	sleep 20


	service couchdb stop &
	pkill -SIGINT tracer
done

