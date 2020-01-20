#!/bin/bash

#stop-dfs.sh
/home/user/apache-hive-2.3.4-bin/bin/hiveserver2 &


while [ 1 ]; do
        [ `jps | grep RunJar | awk '{print $1}'` ] && break || continue
done

sudo /home/user/sscrashanalysis/tracer/src/tracer -p $(jps | grep RunJar | awk '{print $1}') >> /home/user/hive_results/out.txt &
sleep 20


sudo pkill -SIGINT tracer

