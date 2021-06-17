cmake_minimum_required(VERSION 2.8)
project(RECIPE)

set( CMAKE_BUILD_TYPE Debug)
if( NOT CMAKE_BUILD_TYPE )
    message(STATUS "No build type selected, default to Release")
    set( CMAKE_BUILD_TYPE Release)
else()
    message(STATUS "Build type is set to ${CMAKE_BUILD_TYPE}")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -march=native -mrtm -mcx16 -mavx -mavx2 -mbmi2 -mlzcnt -Wno-deprecated-declarations -Wall -Wextra -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free -faligned-new=64 -DNDEBUG")

execute_process(COMMAND cat /proc/cpuinfo COMMAND grep clflush OUTPUT_VARIABLE ENABLE_CLFLUSH)
execute_process(COMMAND cat /proc/cpuinfo COMMAND grep clflushopt OUTPUT_VARIABLE ENABLE_CLFLUSHOPT)
execute_process(COMMAND cat /proc/cpuinfo COMMAND grep clwb OUTPUT_VARIABLE ENABLE_CLWB)
execute_process(COMMAND cat /proc/cpuinfo COMMAND grep avx2 OUTPUT_VARIABLE ENABLE_AVX2)
execute_process(COMMAND cat /proc/cpuinfo COMMAND grep bmi2 OUTPUT_VARIABLE ENABLE_BMI2)
execute_process(COMMAND cat /proc/cpuinfo COMMAND grep avx512 OUTPUT_VARIABLE ENABLE_AVX512)

if(ENABLE_CLWB)
    add_definitions(-DCLWB)
    message(STATUS "Looking for clwb instruction - found")
elseif(ENABLE_CLFLUSHOPT)
    add_definitions(-DCLFLUSH_OPT)
    message(STATUS "Looking for clwb instruction - not found")
    message(STATUS "Looking for clflushopt instruction - found")
elseif(ENABLE_CLFLUSH)
    add_definitions(-DCLFLUSH)
    message(STATUS "Looking for clwb instruction - not found")
    message(STATUS "Looking for clflushopt instruction - not found")
    message(STATUS "Looking for clflush instruction - found")
else()
    message(FATAL_ERROR "Cannot find any flush instructions (clflush, clflushopt, clwb)")
endif()

if(ENABLE_AVX2)
    message(STATUS "Looking for avx2 instruction - found")
else()
    message(STATUS "Looking for avx2 instruction - not found")
endif()

if(ENABLE_BMI2)
    message(STATUS "Looking for bmi2 instruction - found")
else()
    message(STATUS "Looking for bmi2 instruction - not found")
endif()

if(ENABLE_AVX512)
    set(CMAKE_CXX_FLAGS "-mavx512f -mavx512vl -mavx512bw -mavx512dq -mavx512cd ${CMAKE_CXX_FLAGS}")
    add_definitions(-DUSE_AVX512)
    message(STATUS "Looking for avx512 instruction - found")
else()
    message(STATUS "Looking for avx512 instruction - not found")
endif()

### Definitions for CLHT
add_definitions(-DADD_PADDING)
add_definitions(-DDEFAULT) ## CLHT

include_directories(tips/include tips/benchmark tips/src tips/benchmark/include) 

add_subdirectory(tips)

find_library(JemallocLib jemalloc)
find_library(TbbLib tbb)
find_package (Threads)

set(INDEX_FILES nvmm-index/FAST_FAIR/btree.h)

add_library(Indexes ${INDEX_FILES})
link_directories(${CMAKE_SOURCE_DIR}/tips/benchmark/lib)

target_link_libraries(Indexes ${TbbLib} ${JemallocLib} ${CMAKE_THREAD_LIBS_INIT} pthread tipsDS ${CMAKE_SOURCE_DIR}/tips/src/libtips.a pmem pmemobj)

set(YCSB_TEST ycsb.cpp)
add_executable(ycsb ${YCSB_TEST})

target_link_libraries(ycsb Indexes atomic boost_system boost_thread)
