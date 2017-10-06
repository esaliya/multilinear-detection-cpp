#!/bin/bash

#cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=/cm/shared/apps/mpich/ge/gcc/64/3.1/bin/mpicxx -DCMAKE_C_COMPILER=/cm/shared/apps/mpich/ge/gcc/64/3.1/bin/mpicc -DBOOST_ROOT=/home/esaliya/sali/software/boost_1_63_0 -DBOOST_INCLUDEDIR=/home/esaliya/sali/software/builds/boost/include -DBOOST_LIBRARYDIR=/home/esaliya/sali/software/builds/boost/lib ..

cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=/home/esaliya/intel/compilers_and_libraries_2018.0.128/linux/mpi/intel64/bin/mpicxx -DCMAKE_C_COMPILER=/home/esaliya/intel/compilers_and_libraries_2018.0.128/linux/mpi/intel64/bin/mpicc -DBOOST_ROOT=/home/esaliya/sali/software/boost_1_64_0 -DBOOST_INCLUDEDIR=/home/esaliya/sali/software/builds/boost/include -DBOOST_LIBRARYDIR=/home/esaliya/sali/software/builds/boost/lib ..

make
