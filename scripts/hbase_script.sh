#!/bin/bash

#stop-hbase.sh
start-hbase.sh &


while [ 1 ]; do
        [ `jps | grep HMaster | awk '{print $1}'` ] && break || continue
done    

sudo /home/user/sscrashanalysis/tracer/src/tracer -p $(jps | grep HMaster | awk '{print $1}') >> /home/user/hbase_results/out.txt &
sleep 20


stop-hbase.sh &
sudo pkill -SIGINT tracer
