
cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(TTreeProcessor)

list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})

add_definitions(-std=c++1z)

find_package(ROOT REQUIRED COMPONENTS RIO)

add_executable(testProcessor src/test.cxx)
target_link_libraries(testProcessor ${ROOT_LIBRARIES})

