//
// Created by Scott Brumbaugh on 8/9/16.
//

#include "gtest/gtest.h"

#include "packet.h"
#include "packet_data.h"

TEST(packets, read_remaining_length) {

    std::vector<uint8_t> packet_data(4);
    packet_data[0] = 127;

    PacketDataReader reader1(packet_data);
    ASSERT_TRUE(reader1.has_remaining_length());
    ASSERT_EQ(reader1.read_remaining_length(), 127);

    packet_data[0] = 128 + 10;
    packet_data[1] = 127;
    PacketDataReader reader2(packet_data);
    ASSERT_TRUE(reader2.has_remaining_length());
    ASSERT_EQ(reader2.read_remaining_length(), 10 + 128 * 127);

    packet_data[0] = 128 + 10;
    packet_data[1] = 128 + 10;
    packet_data[2] = 127;
    PacketDataReader reader3(packet_data);
    ASSERT_TRUE(reader3.has_remaining_length());
    ASSERT_EQ(reader3.read_remaining_length(), 10 + 128 * 10 + 128 * 128 * 127);

    packet_data[0] = 128 + 10;
    packet_data[1] = 128 + 10;
    packet_data[2] = 128 + 10;
    packet_data[3] = 127;
    PacketDataReader reader4(packet_data);
    ASSERT_TRUE(reader4.has_remaining_length());
    ASSERT_EQ(reader4.read_remaining_length(), 10 + 128 * 10 + 128 * 128 * 10 + 128 * 128 * 128 * 127);

    packet_data[0] = 128;
    packet_data[1] = 128;
    packet_data[2] = 128;
    packet_data[3] = 128;
    PacketDataReader reader5(packet_data);
    ASSERT_FALSE(reader5.has_remaining_length());

}

TEST(packets, write_remaining_length) {

    std::vector<uint8_t> packet_data(4, 0);
    size_t remaining_length;

    PacketDataWriter writer1(packet_data);
    remaining_length = 127;
    writer1.write_remaining_length(remaining_length);
    ASSERT_EQ(packet_data[0], 127);
    ASSERT_EQ(packet_data[1], 0);

    PacketDataWriter writer2(packet_data);
    remaining_length = 10 + 128 * 127;
    writer2.write_remaining_length(remaining_length);
    ASSERT_EQ(packet_data[0], 128 + 10);
    ASSERT_EQ(packet_data[1], 127);
    ASSERT_EQ(packet_data[2], 0);

    PacketDataWriter writer3(packet_data);
    remaining_length = 10 + 128 * 10 + 128 * 128 * 127;
    writer3.write_remaining_length(remaining_length);
    ASSERT_EQ(packet_data[0], 128 + 10);
    ASSERT_EQ(packet_data[1], 128 + 10);
    ASSERT_EQ(packet_data[2], 127);
    ASSERT_EQ(packet_data[3], 0);

    PacketDataWriter writer4(packet_data);
    remaining_length = 10 + 128 * 10 + 128 * 128 * 10 + 128 * 128 * 128 * 127;
    writer3.write_remaining_length(remaining_length);
    ASSERT_EQ(packet_data[0], 128 + 10);
    ASSERT_EQ(packet_data[1], 128 + 10);
    ASSERT_EQ(packet_data[2], 128 + 10);
    ASSERT_EQ(packet_data[3], 127);

    PacketDataWriter writer5(packet_data);
    remaining_length = 127 + 128 * 127 + 128 * 128 * 127 + 128 * 128 * 128 * 127;
    writer5.write_remaining_length(remaining_length);
    ASSERT_EQ(packet_data[0], 0xFF);
    ASSERT_EQ(packet_data[1], 0xFF);
    ASSERT_EQ(packet_data[2], 0xFF);
    ASSERT_EQ(packet_data[3], 0x7F);

    PacketDataWriter writer6(packet_data);
    remaining_length = 127 + 128 * 127 + 128 * 128 * 127 + 128 * 128 * 128 * 127 + 1;
    ASSERT_THROW(writer6.write_remaining_length(remaining_length), std::exception);

}

TEST(packets, connect_packet) {

    ConnectPacket connect_packet1;

    connect_packet1.protocol_name = "MQIdsp";
    connect_packet1.protocol_level = 4;
    connect_packet1.client_id = "client1";
    connect_packet1.clean_session(true);
    connect_packet1.will_flag(true);
    connect_packet1.qos(2);
    connect_packet1.keep_alive = 60;
    connect_packet1.will_retain(true);
    connect_packet1.password_flag(true);
    connect_packet1.username_flag(true);

    connect_packet1.will_topic = "will_topic";
    connect_packet1.will_message = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    connect_packet1.username = "username";
    connect_packet1.password = {9, 8, 7, 6, 5, 4, 3, 2, 1, 0};

    std::vector<uint8_t> packet_data = connect_packet1.serialize();

    ConnectPacket connect_packet2(packet_data);

    ASSERT_EQ(connect_packet1.protocol_name, connect_packet2.protocol_name);
    ASSERT_EQ(connect_packet1.protocol_level, connect_packet2.protocol_level);
    ASSERT_EQ(connect_packet1.client_id, connect_packet2.client_id);
    ASSERT_EQ(connect_packet1.clean_session(), connect_packet2.clean_session());
    ASSERT_EQ(connect_packet1.will_flag(), connect_packet2.will_flag());
    ASSERT_EQ(connect_packet1.qos(), connect_packet2.qos());
    ASSERT_EQ(connect_packet1.keep_alive, connect_packet2.keep_alive);
    ASSERT_EQ(connect_packet1.will_retain(), connect_packet2.will_retain());
    ASSERT_EQ(connect_packet1.password_flag(), connect_packet2.password_flag());
    ASSERT_EQ(connect_packet1.username_flag(), connect_packet2.username_flag());
    ASSERT_EQ(connect_packet1.will_topic, connect_packet2.will_topic);
    ASSERT_EQ(connect_packet1.will_message, connect_packet2.will_message);
    ASSERT_EQ(connect_packet1.username, connect_packet2.username);
    ASSERT_EQ(connect_packet1.password, connect_packet2.password);

}

TEST(packets, connack_packet) {

    ConnackPacket connack_packet1;

    connack_packet1.acknowledge_flags = 0x01;
    connack_packet1.return_code = ConnackPacket::ReturnCode::Accepted;

    std::vector<uint8_t> packet_data = connack_packet1.serialize();

    ConnackPacket connack_packet2(packet_data);

    ASSERT_EQ(connack_packet2.acknowledge_flags, connack_packet1.acknowledge_flags);
    ASSERT_EQ(connack_packet2.return_code, connack_packet1.return_code);
}

TEST(packets, publish_packet) {

    PublishPacket publish_packet1;

    publish_packet1.dup(true);
    publish_packet1.qos(2);
    publish_packet1.retain(true);

    publish_packet1.topic_name = "test_topic";
    publish_packet1.packet_id = 100;
    publish_packet1.message_data = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    std::vector<uint8_t> packet_data = publish_packet1.serialize();

    PublishPacket publish_packet2(packet_data);

    ASSERT_EQ(publish_packet2.dup(), publish_packet1.dup());
    ASSERT_EQ(publish_packet2.qos(), publish_packet1.qos());
    ASSERT_EQ(publish_packet2.retain(), publish_packet1.retain());
    ASSERT_EQ(publish_packet2.topic_name, publish_packet1.topic_name);
    ASSERT_EQ(publish_packet2.packet_id, publish_packet1.packet_id);
    ASSERT_EQ(publish_packet2.message_data, publish_packet1.message_data);

}

TEST(packets, puback_packet) {

    PubackPacket puback_packet1;

    puback_packet1.packet_id = 100;

    std::vector<uint8_t> packet_data = puback_packet1.serialize();

    PubackPacket puback_packet2(packet_data);

    ASSERT_EQ(puback_packet2.packet_id, puback_packet1.packet_id);

}

TEST(packets, pubrec_packet) {

    PubrecPacket pubrec_packet1;

    pubrec_packet1.packet_id = 100;

    std::vector<uint8_t> packet_data = pubrec_packet1.serialize();

    PubrecPacket pubrec_packet2(packet_data);

    ASSERT_EQ(pubrec_packet2.packet_id, pubrec_packet1.packet_id);

}

TEST(packets, pubrel_packet) {

    PubrelPacket pubrel_packet1;

    pubrel_packet1.packet_id = 100;

    std::vector<uint8_t> packet_data = pubrel_packet1.serialize();

    PubrelPacket pubrel_packet2(packet_data);

    ASSERT_EQ(pubrel_packet2.packet_id, pubrel_packet1.packet_id);

}

TEST(packets, pubcomp_packet) {

    PubcompPacket pubcomp_packet1;

    pubcomp_packet1.packet_id = 100;

    std::vector<uint8_t> packet_data = pubcomp_packet1.serialize();

    PubcompPacket pubcomp_packet2(packet_data);

    ASSERT_EQ(pubcomp_packet2.packet_id, pubcomp_packet1.packet_id);

}

TEST(packets, subscribe_packet) {

    SubscribePacket subscribe_packet1;

    subscribe_packet1.packet_id = 100;
    subscribe_packet1.subscriptions = {
            {TopicFilter("subscription1"), 0},
            {TopicFilter("subscription2"), 1},
            {TopicFilter("subscription3"), 2}
    };

    std::vector<uint8_t> packet_data = subscribe_packet1.serialize();

    SubscribePacket subscribe_packet2(packet_data);

    ASSERT_EQ(subscribe_packet2.packet_id, subscribe_packet1.packet_id);
    ASSERT_EQ(subscribe_packet2.subscriptions.size(), subscribe_packet1.subscriptions.size());

    for (int i = 0; i < subscribe_packet2.subscriptions.size(); i++) {
        ASSERT_TRUE(topic_match(subscribe_packet2.subscriptions[i].topic_filter,
                                subscribe_packet1.subscriptions[i].topic_filter));
        ASSERT_EQ(subscribe_packet2.subscriptions[i].qos, subscribe_packet2.subscriptions[i].qos);
    }

}

TEST(packets, suback_packet) {

    SubackPacket suback_packet1;

    suback_packet1.packet_id = 100;

    suback_packet1.return_codes = {
            SubackPacket::ReturnCode::SuccessQoS0,
            SubackPacket::ReturnCode::SuccessQoS1,
            SubackPacket::ReturnCode::SuccessQoS2,
            SubackPacket::ReturnCode::Failure,
    };

    std::vector<uint8_t> packet_data = suback_packet1.serialize();

    SubackPacket suback_packet2(packet_data);

    ASSERT_EQ(suback_packet2.packet_id, suback_packet1.packet_id);

    ASSERT_EQ(suback_packet2.return_codes.size(), suback_packet1.return_codes.size());

    for (int i = 0; i < suback_packet2.return_codes.size(); i++) {
        ASSERT_EQ(suback_packet2.return_codes[i], suback_packet1.return_codes[i]);
    }

}

TEST(packets, unsubscribe_packet) {

    UnsubscribePacket unsubscribe_packet1;

    unsubscribe_packet1.packet_id = 100;

    unsubscribe_packet1.topics = {
            {"a/b"},
            {"c/d"},
            {"#"}
    };

    std::vector<uint8_t> packet_data = unsubscribe_packet1.serialize();

    UnsubscribePacket unsubscribe_packet2(packet_data);

    ASSERT_EQ(unsubscribe_packet2.packet_id, unsubscribe_packet1.packet_id);

    ASSERT_EQ(unsubscribe_packet2.topics.size(), unsubscribe_packet1.topics.size());

    for (int i = 0; i < unsubscribe_packet2.topics.size(); i++) {
        ASSERT_EQ(unsubscribe_packet2.topics[i], unsubscribe_packet1.topics[i]);
    }
}

TEST(packets, unsuback_packet) {

    UnsubackPacket unsuback_packet1;

    unsuback_packet1.packet_id = 100;

    std::vector<uint8_t> packet_data = unsuback_packet1.serialize();

    UnsubackPacket unsuback_packet2(packet_data);

    ASSERT_EQ(unsuback_packet2.packet_id, unsuback_packet1.packet_id);

}

TEST(packets, pingreq_packet) {

    PingreqPacket pingreq_packet1;

    std::vector<uint8_t> packet_data = pingreq_packet1.serialize();

    PingreqPacket pingreq_packet2(packet_data);

    ASSERT_EQ(pingreq_packet2.type, pingreq_packet1.type);

}

TEST(packets, pingresp_packet) {

    PingrespPacket pingresp_packet1;

    std::vector<uint8_t> packet_data = pingresp_packet1.serialize();

    PingrespPacket pingresp_packet2(packet_data);

    ASSERT_EQ(pingresp_packet2.type, pingresp_packet1.type);

}

TEST(packets, disconnect_packet) {

    DisconnectPacket disconnect_packet1;

    std::vector<uint8_t> packet_data = disconnect_packet1.serialize();

    DisconnectPacket disconnect_packet2(packet_data);

    ASSERT_EQ(disconnect_packet2.type, disconnect_packet1.type);

}