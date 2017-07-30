#!/bin/bash
mpiexec -np 4 -ppn 2 -f ~/nodes.2n.b.txt ../build/main_omp
