#!/bin/bash
export I_MPI_CXX=/N/u/sekanaya/intel/compilers_and_libraries_2016.2.181/linux/bin/intel64/icpc
export I_MPI_CC=/N/u/sekanaya/intel/compilers_and_libraries_2016.2.181/linux/bin/intel64/icc

#cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=/cm/shared/apps/mpich/ge/gcc/64/3.1/bin/mpicxx -DCMAKE_C_COMPILER=/cm/shared/apps/mpich/ge/gcc/64/3.1/bin/mpicc -DBOOST_ROOT=/home/esaliya/sali/software/boost_1_63_0 -DBOOST_INCLUDEDIR=/home/esaliya/sali/software/builds/boost/include -DBOOST_LIBRARYDIR=/home/esaliya/sali/software/builds/boost/lib ..

cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=/N/u/sekanaya/intel/compilers_and_libraries_2016.2.181/linux/mpi/intel64/bin/mpicxx -DCMAKE_C_COMPILER=/N/u/sekanaya/intel/compilers_and_libraries_2016.2.181/linux/mpi/intel64/bin/mpicc -DBOOST_ROOT=/N/u/sekanaya/sali/software/boost_1_64_0 -DBOOST_INCLUDEDIR=/N/u/sekanaya/sali/software/builds/boost/include -DBOOST_LIBRARYDIR=/N/u/sekanaya/sali/software/builds/boost/lib ..

make
