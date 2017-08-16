#!/bin/bash
vc=7
k=3
delta=1
alpha=0.15
epsilon=0.1
input_file="/Users/esaliya/sali/git/github/esaliya/ccpp/multilinear-detection-cpp-copy/resources/my_little_graph_simple.txt"
node_count=1
thread_count=1

#procs=3
#mpiexec -np $procs ../cmake-build-debug/main --v $vc --k $k --d $delta --a $alpha --e $epsilon --i $input_file --nc $node_count --tc $thread_count --out out.txt --mms 1  --ibs 4

#alltoallv-worlds
procs=1
mpiexec -np $procs ../cmake-build-debug/main --v $vc --k $k --d $delta --a $alpha --e $epsilon --i $input_file --nc $node_count --out out.txt --mms 20  --ibs 1 --pic 1


