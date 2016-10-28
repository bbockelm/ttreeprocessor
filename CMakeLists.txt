
cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(TTreeProcessor)

list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})

add_definitions(-std=c++1z)
add_definitions(-O3)

find_package(ROOT REQUIRED COMPONENTS RIO)

include_directories( src )

add_executable(testProcessor src/test.cxx)
add_executable(testBackports src/test/testBackports.cxx)
#add_executable(testProcessor src/test.cxx)
#target_link_libraries(testProcessor ${ROOT_LIBRARIES})

