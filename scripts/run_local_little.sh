#!/bin/bash

#$1 = nprocs
#$2 = ibs

vc=7
template_vc=4
delta=1
alpha=0.15
epsilon=0.1
input_file="/Users/esaliya/sali/git/github/esaliya/ccpp/multilinear-detection-cpp-trees/resources/my_little_graph_simple.txt"
template_file="/Users/esaliya/sali/git/github/esaliya/ccpp/multilinear-detection-cpp-trees/resources/my_little_template.txt"
out_file="/Users/esaliya/sali/git/github/esaliya/ccpp/multilinear-detection-cpp-trees/resources/out.txt"
ibs=$2
node_count=1

procs=$1
mpiexec -np $procs ../cmake-build-debug/main --v $vc --tvc $template_vc --d $delta --a $alpha --e $epsilon --i $input_file --t $template_file --out $out_file --ibs $ibs --nc $node_count --mms 1



