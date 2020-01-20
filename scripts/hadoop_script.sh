#!/bin/bash

#stop-dfs.sh
start-dfs.sh &


while [ 1 ]; do
        [ `jps | grep NameNode | awk '{print $1}'` ] && break || continue
done

sudo /home/user/sscrashanalysis/tracer/src/tracer -p $(jps | grep NameNode | awk '{print $1}') >> /home/user/hadoop_results/out.txt &
sleep 20


stop-dfs.sh &
sudo pkill -SIGINT tracer


