
add_executable(run_tests topic_tests.cc packet_tests.cc protocol_tests.cc)

include_directories(run_tests ${CMAKE_SOURCE_DIR} ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

target_link_libraries(run_tests mqtt gtest gtest_main event)
