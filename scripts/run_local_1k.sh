#!/bin/bash
vc=804
k=6
delta=1
alpha=0.15
epsilon=0.1
input_file="/Users/esaliya/sali/projects/graphs/giraph/data/random-er/0001000.txt"
node_count=1
thread_count=1

procs=9
mpiexec -np $procs ../cmake-build-debug/main --v $vc --k $k --d $delta --a $alpha --e $epsilon --i $input_file --nc $node_count --tc $thread_count


