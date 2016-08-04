cmake_minimum_required(VERSION 3.5.1)
PROJECT(mqtt)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -std=c++11")

SET(SOURCES main.cc session.cc packet.cc packet_manager.cc packet_data.cc client_id.cc client_id.h)

INCLUDE_DIRECTORIES(/opt/local/include)
LINK_DIRECTORIES(/opt/local/lib)

ADD_EXECUTABLE(mqtt ${SOURCES})
TARGET_LINK_LIBRARIES(mqtt event)




