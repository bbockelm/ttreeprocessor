
cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(TTreeProcessor)

list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

add_definitions(-std=c++14)
add_definitions(-O3)
add_definitions(-g)

find_package(ROOT REQUIRED COMPONENTS RIO TreePlayer)
include(${ROOT_USE_FILE})

find_package(TBB REQUIRED)

include_directories( src )

ROOT_GENERATE_DICTIONARY(G__Event src/test/event/Event.h LINKDEF src/test/event/EventLinkDef.h)
add_library(Event SHARED src/test/event/Event.cxx G__Event.cxx)
target_link_libraries(Event ${ROOT_LIBRARIES})
add_executable(testEvent src/test/MainEvent.cxx)
target_link_libraries(testEvent Event)
target_include_directories(testEvent PRIVATE src/test/event)

ROOT_GENERATE_DICTIONARY(G__SillyStruct src/test/SillyStruct.h LINKDEF src/test/SillyStructLinkDef.h)
add_library(SillyStruct SHARED src/test/SillyStruct.cxx G__SillyStruct.cxx)
target_link_libraries(SillyStruct ${ROOT_LIBRARIES})
add_executable(testSillyStruct src/test/MainSillyStruct.cxx)
target_link_libraries(testSillyStruct SillyStruct)

add_executable(testProcessor src/test/testProcessor.cxx)
target_link_libraries(testProcessor ${ROOT_LIBRARIES} ${TBB_LIBRARIES})
target_include_directories(testProcessor PRIVATE ${TBB_INCLUDE_DIRS})

add_executable(testProcessorParallel src/test/testProcessorParallel.cxx)
target_link_libraries(testProcessorParallel ${ROOT_LIBRARIES} ${TBB_LIBRARIES})
target_include_directories(testProcessorParallel PRIVATE ${TBB_INCLUDE_DIRS})

add_executable(testProcessorSimple src/test/testProcessorSimple.cxx)
target_link_libraries(testProcessorSimple ${ROOT_LIBRARIES} ${TBB_LIBRARIES})
target_include_directories(testProcessorSimple PRIVATE ${TBB_INCLUDE_DIRS})

add_executable(testBackports src/test/testBackports.cxx)
add_executable(testHelpers   src/test/testHelpers.cxx)
add_executable(testRootHelpers src/test/testRootHelpers.cxx)
target_link_libraries(testRootHelpers ${ROOT_LIBRARIES})
#target_link_libraries(testProcessor ${ROOT_LIBRARIES})

