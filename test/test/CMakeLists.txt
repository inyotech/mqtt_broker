ADD_EXECUTABLE(run_tests topic_tests.cc packet_tests.cc protocol_tests.cc session_tests.cc)

INCLUDE_DIRECTORIES(run_tests ${CMAKE_SOURCE_DIR}/src ${LIBEVENT_INCLUDE_DIR} ${gtest_SOURCE_DIR}/include
        ${gtest_SOURCE_DIR})

INCLUDE_DIRECTORIES(${LIBEVENT_INCLUDE_DIR})

TARGET_LINK_LIBRARIES(run_tests mqtt gtest gtest_main ${LIBEVENT_LIB})
