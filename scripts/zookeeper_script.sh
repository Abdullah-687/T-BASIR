#!/bin/bash


/home/user/zookeeper-3.4.12/bin/zkServer.sh start &


while [ 1 ]; do
        [ `jps | grep QuorumPeerMain | awk '{print $1}'` ] && break || continue
done    

/home/user/sscrashanalysis/tracer/src/tracer -p $(jps | grep QuorumPeerMain | awk '{print $1}') >> /home/user/zookeeper_results/out.txt &
sleep 20


/home/user/zookeeper-3.4.12/bin/zkServer.sh stop &
pkill -SIGINT tracer
