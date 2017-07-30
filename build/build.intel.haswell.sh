#!/bin/bash
export I_MPI_CXX=/cm/shared/apps/intel/composer_xe/2015.5.223/bin/intel64/icpc
export I_MPI_CC=/cm/shared/apps/intel/composer_xe/2015.5.223/bin/intel64/icc

#cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=/cm/shared/apps/mpich/ge/gcc/64/3.1/bin/mpicxx -DCMAKE_C_COMPILER=/cm/shared/apps/mpich/ge/gcc/64/3.1/bin/mpicc -DBOOST_ROOT=/home/esaliya/sali/software/boost_1_63_0 -DBOOST_INCLUDEDIR=/home/esaliya/sali/software/builds/boost/include -DBOOST_LIBRARYDIR=/home/esaliya/sali/software/builds/boost/lib ..

cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=/home/esaliya/intel/compilers_and_libraries/linux/mpi/bin64/mpicxx -DCMAKE_C_COMPILER=/home/esaliya/intel/compilers_and_libraries/linux/mpi/bin64/mpicc -DBOOST_ROOT=/home/esaliya/sali/software/boost_1_64_0 -DBOOST_INCLUDEDIR=/home/esaliya/sali/software/builds/boost/include -DBOOST_LIBRARYDIR=/home/esaliya/sali/software/builds/boost/lib ..

make
