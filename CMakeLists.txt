cmake_minimum_required(VERSION 3.5.1)
PROJECT(mqtt)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Werror -std=c++11")

SET(LIB_SOURCES session.cc packet.cc packet_manager.cc packet_data.cc client_id.cc topic.cc
        session_manager.cc)

ADD_LIBRARY(mqtt STATIC ${LIB_SOURCES})

INCLUDE_DIRECTORIES(/opt/local/include)
LINK_DIRECTORIES(/opt/local/lib)

ADD_EXECUTABLE(mqtt_broker main.cc)
TARGET_LINK_LIBRARIES(mqtt_broker mqtt event)

ADD_EXECUTABLE(mqtt_client_pub client_pub.cc)
TARGET_LINK_LIBRARIES(mqtt_client_pub mqtt event)

ADD_EXECUTABLE(mqtt_client_sub client_sub.cc)
TARGET_LINK_LIBRARIES(mqtt_client_sub mqtt event)

ADD_SUBDIRECTORY(test)

