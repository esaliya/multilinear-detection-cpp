#!/bin/bash

cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=/cm/shared/apps/mpich/ge/gcc/64/3.2rc2/bin/mpicxx -DCMAKE_C_COMPILER=/cm/shared/apps/mpich/ge/gcc/64/3.2rc2/bin/mpicc -DBOOST_ROOT=/home/esaliya/sali/software/boost_1_64_0 -DBOOST_INCLUDEDIR=/home/esaliya/sali/software/builds/boost/include -DBOOST_LIBRARYDIR=/home/esaliya/sali/software/builds/boost/lib ..

make
