SET(LIB_SOURCES session_base.cc session.cc packet.cc packet_manager.cc packet_data.cc client_id.cc topic.cc
        session_manager.cc)

ADD_LIBRARY(mqtt STATIC ${LIB_SOURCES})

ADD_EXECUTABLE(mqtt_broker broker.cc)
TARGET_LINK_LIBRARIES(mqtt_broker mqtt event)

ADD_EXECUTABLE(mqtt_client_pub client_pub.cc)
TARGET_LINK_LIBRARIES(mqtt_client_pub mqtt event)

ADD_EXECUTABLE(mqtt_client_sub client_sub.cc)
TARGET_LINK_LIBRARIES(mqtt_client_sub mqtt event)
