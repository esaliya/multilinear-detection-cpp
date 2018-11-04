cmake_minimum_required(VERSION 3.7)
project(clioncpp)


find_package(MPI REQUIRED)
find_package(OpenMP REQUIRED)
find_package(Boost 1.63 COMPONENTS program_options REQUIRED)



set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

set(SOURCE_FILES main.cpp parallel_ops.cpp parallel_ops.hpp constants.h constants.cpp vertex.hpp message.hpp vertex_buffer.hpp recv_vertex_buffer.hpp galois_field.hpp galois_field.cpp polynomial.cpp polynomial.hpp polynomials.hpp utils.hpp template/graph.cpp template/graph.h template/template_partitioner.h)
add_executable(main ${SOURCE_FILES})

#set(SOURCE_FILES main_dummy.cpp constants.h constants.cpp)
#add_executable(main_dummy ${SOURCE_FILES})

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(main ${Boost_LIBRARIES})
endif()

if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

if (MPI_FOUND)
    include_directories(${MPI_INCLUDE_PATH})
    target_link_libraries(main ${MPI_LIBRARIES})
endif()

#set(SOURCE_FILES main_test.cpp test_map_class.cpp test_map_class.hpp map_constructor.cpp map_constructor.hpp polynomial.cpp polynomial.hpp template/graph.cpp template/graph.h template/template_partitioner.h)
#add_executable(main_test ${SOURCE_FILES})

set(SOURCE_FILES main_omp.cpp)
add_executable(main_omp ${SOURCE_FILES})

#set(SOURCE_FILES metis_converter.cpp)
#add_executable(metis_converter ${SOURCE_FILES})

