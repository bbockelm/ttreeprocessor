
ROOT_GENERATE_DICTIONARY(G__Event event/Event.h LINKDEF event/EventLinkDef.h)
add_library(Event SHARED event/Event.cxx G__Event.cxx)
target_link_libraries(Event ${ROOT_LIBRARIES})
add_executable(testEvent MainEvent.cxx)
target_link_libraries(testEvent Event)
target_include_directories(testEvent PRIVATE event)

ROOT_GENERATE_DICTIONARY(G__SillyStruct SillyStruct.h LINKDEF SillyStructLinkDef.h)
add_library(SillyStruct SHARED SillyStruct.cxx G__SillyStruct.cxx)
target_link_libraries(SillyStruct ${ROOT_LIBRARIES})
add_executable(testSillyStruct MainSillyStruct.cxx)
target_link_libraries(testSillyStruct SillyStruct)

add_executable(testProcessor testProcessor.cxx)
target_link_libraries(testProcessor ${ROOT_LIBRARIES} ${TBB_LIBRARIES} ${Vc_LIBRARIES})

add_executable(testProcessorParallel testProcessorParallel.cxx)
target_link_libraries(testProcessorParallel ${ROOT_LIBRARIES} ${TBB_LIBRARIES} ${Vc_LIBRARIES})

add_executable(testProcessorSimple testProcessorSimple.cxx)
target_link_libraries(testProcessorSimple ${ROOT_LIBRARIES} ${TBB_LIBRARIES} ${Vc_LIBRARIES})

add_executable(testProcessorVectorized testProcessorVectorized.cxx)
target_link_libraries(testProcessorVectorized ${ROOT_LIBRARIES} ${TBB_LIBRARIES} ${Vc_LIBRARIES})

add_executable(testBackports testBackports.cxx)
add_executable(testHelpers   testHelpers.cxx)
add_executable(testRootHelpers testRootHelpers.cxx)
target_link_libraries(testRootHelpers ${ROOT_LIBRARIES} ${Vc_LIBRARIES})

add_executable(testVcHelpers testVcHelpers.cxx)
target_link_libraries(testVcHelpers ${Vc_LIBRARIES})

