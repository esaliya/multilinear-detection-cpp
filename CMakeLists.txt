cmake_minimum_required(VERSION 3.7)
project(clioncpp)


find_package(MPI REQUIRED)
find_package(Boost 1.63.0 COMPONENTS program_options REQUIRED)



set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=gnu++11 -fopenmp")

set(SOURCE_FILES main.cpp parallel_ops.cpp parallel_ops.hpp constants.h constants.cpp vertex.hpp graph.cpp graph.hpp message.hpp vertex_buffer.hpp recv_vertex_buffer.hpp galois_field.hpp galois_field.cpp polynomial.cpp polynomial.hpp polynomials.hpp utils.hpp)
add_executable(main ${SOURCE_FILES})

#set(SOURCE_FILES main_dummy.cpp constants.h constants.cpp)
#add_executable(main_dummy ${SOURCE_FILES})

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(main ${Boost_LIBRARIES})
endif()

set(SOURCE_FILES main_test.cpp test_map_class.cpp test_map_class.hpp map_constructor.cpp map_constructor.hpp polynomial.cpp polynomial.hpp)
add_executable(main_test ${SOURCE_FILES})

set(SOURCE_FILES main_omp.cpp)
add_executable(main_omp ${SOURCE_FILES})

