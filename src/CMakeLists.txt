SET(LIB_SOURCES session_base.cc session.cc packet.cc packet_manager.cc packet_data.cc client_id.cc topic.cc
        session_manager.cc)

ADD_LIBRARY(mqtt STATIC ${LIB_SOURCES})

INCLUDE_DIRECTORIES(${LIBEVENT_INCLUDE_DIR})

ADD_EXECUTABLE(mqtt_broker broker.cc)
TARGET_LINK_LIBRARIES(mqtt_broker mqtt ${LIBEVENT_LIB})

ADD_EXECUTABLE(mqtt_client_pub client_pub.cc)
TARGET_LINK_LIBRARIES(mqtt_client_pub mqtt ${LIBEVENT_LIB})

ADD_EXECUTABLE(mqtt_client_sub client_sub.cc)
TARGET_LINK_LIBRARIES(mqtt_client_sub mqtt ${LIBEVENT_LIB})
