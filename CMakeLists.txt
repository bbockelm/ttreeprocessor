
cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(TTreeProcessor)

list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
list(APPEND CMAKE_PREFIX_PATH $ENV{VC_BUILD}/lib/cmake)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

add_definitions(-std=c++14)
add_definitions(-O3)
#add_definitions(-g)
add_definitions(-flto)
add_definitions(-march=native)
add_definitions(-Wno-ignored-attributes)

set (CMAKE_EXE_LINKER_FLAGS "-flto -fwhole-program -O3")

find_package(ROOT REQUIRED COMPONENTS RIO TreePlayer)
include(${ROOT_USE_FILE})

find_package(TBB REQUIRED)
find_package(Vc REQUIRED)

include_directories( src ${TBB_INCLUDE_DIRS} ${Vc_INCLUDE_DIR})

add_subdirectory(src/test)

