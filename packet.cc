#include "packet.h"
#include "client_id.h"
#include "make_unique.h"

#include <cstring>
#include <iostream>
#include <cassert>

#include <event2/buffer.h>


ConnectPacket::ConnectPacket(uint8_t command, const std::vector<uint8_t> &packet_data) {

    assert(static_cast<PacketType>(command >> 4) == PacketType::Connect);

    type = PacketType::Connect;
    header_flags = command & 0x0F;

    PacketDataReader reader(packet_data);

    protocol_name = reader.read_string();
    protocol_level = reader.read_byte();
    connect_flags = reader.read_byte();
    keep_alive = reader.read_uint16();
    client_id = reader.read_string();

    if (client_id.empty()) {
        client_id = generate_client_id();
    }

    if (will_flag()) {
        will_topic = reader.read_string();
        will_message = reader.read_bytes();
    }

    if (username_flag()) {
        username = reader.read_string();
    }

    if (password_flag()) {
        password = reader.read_bytes();
    }

}

std::vector<uint8_t> ConnectPacket::serialize() const {
    std::vector<uint8_t> packet_data;
    PacketDataWriter writer(packet_data);
    return packet_data;
}


std::vector<uint8_t> ConnackPacket::serialize() const {

    std::vector<uint8_t> packet_data;
    PacketDataWriter writer(packet_data);

    writer.write_byte((static_cast<uint8_t>(type) << 4) | (header_flags & 0x0F));
    writer.write_remaining_length(2);
    writer.write_byte(acknowledge_flags);
    writer.write_byte(static_cast<uint8_t>(return_code));
    return packet_data;
}

PublishPacket::PublishPacket(uint8_t command, const std::vector<uint8_t> &packet_data) {

    assert(static_cast<PacketType>(command >> 4) == PacketType::Publish);

    type = PacketType::Publish;
    header_flags = command & 0x0F;

    PacketDataReader reader(packet_data);

    topic_name = reader.read_string();

    if (qos() != 0) {
        packet_id = reader.read_uint16();
    }

    size_t header_len = topic_name.size() + 2;
    if (qos() != 0) {
        header_len += 2;
    }

    size_t payload_len = packet_data.size() - header_len;

    message = reader.read_bytes(payload_len);

}

std::vector<uint8_t> PublishPacket::serialize() const {
    std::vector<uint8_t> packet_data;
    PacketDataWriter writer(packet_data);
    return packet_data;
}

PubackPacket::PubackPacket(uint8_t command, const std::vector<uint8_t> &packet_data) {
    assert(static_cast<PacketType>(command >> 4) == PacketType::Publish);

    type = PacketType::Publish;
    header_flags = command & 0x0F;

    PacketDataReader reader(packet_data);

    packet_id = reader.read_uint16();

}

std::vector<uint8_t> PubackPacket::serialize() const {

    std::vector<uint8_t> packet_data;
    PacketDataWriter writer(packet_data);
    writer.write_byte((static_cast<uint8_t>(type) << 4) | (header_flags & 0x0F));
    writer.write_remaining_length(2);
    writer.write_uint16(packet_id);
    return packet_data;
}

PubrecPacket::PubrecPacket(uint8_t command, const std::vector<uint8_t> &packet_data) {
    assert(static_cast<PacketType>(command >> 4) == PacketType::Pubrec);

    type = PacketType::Pubrec;
    header_flags = command & 0x0F;

    PacketDataReader reader(packet_data);

    packet_id = reader.read_uint16();
}

std::vector<uint8_t> PubrecPacket::serialize() const {

    std::vector<uint8_t> packet_data;
    PacketDataWriter writer(packet_data);
    writer.write_byte((static_cast<uint8_t>(type) << 4) | (header_flags & 0x0F));
    writer.write_remaining_length(2);
    writer.write_uint16(packet_id);
    return packet_data;
}

PubrelPacket::PubrelPacket(uint8_t command, const std::vector<uint8_t> &packet_data) {

    std::cout << "pubrel\n";

    assert(static_cast<PacketType>(command >> 4) == PacketType::Pubrel);
    assert((command & 0x0F) == 0x02);

    type = PacketType::Pubrel;

    header_flags = command & 0x0F;

    PacketDataReader reader(packet_data);

    packet_id = reader.read_uint16();
}

std::vector<uint8_t> PubrelPacket::serialize() const {

    std::vector<uint8_t> packet_data;
    PacketDataWriter writer(packet_data);
    writer.write_byte((static_cast<uint8_t>(type) << 4) | (header_flags & 0x0F));
    writer.write_remaining_length(2);
    writer.write_uint16(packet_id);
    return packet_data;
}

PubcompPacket::PubcompPacket(uint8_t command, const std::vector<uint8_t> &packet_data) {
    assert(static_cast<PacketType>(command >> 4) == PacketType::Pubcomp);
    assert((command & 0x0F) == 0x02);

    type = PacketType::Pubcomp;

    header_flags = command & 0x0F;

    PacketDataReader reader(packet_data);

    packet_id = reader.read_uint16();
}

std::vector<uint8_t> PubcompPacket::serialize() const {

    std::vector<uint8_t> packet_data;
    PacketDataWriter writer(packet_data);
    writer.write_byte((static_cast<uint8_t>(type) << 4) | (header_flags & 0x0F));
    writer.write_remaining_length(2);
    writer.write_uint16(packet_id);
    return packet_data;
}

SubscribePacket::SubscribePacket(uint8_t command, const std::vector<uint8_t> &packet_data) {
    assert(static_cast<PacketType>(command >> 4) == PacketType::Subscribe);
    assert((command & 0x0F) == 0x02);

    type = PacketType::Subscribe;

    header_flags = command & 0x0F;

    PacketDataReader reader(packet_data);

    packet_id = reader.read_uint16();

    do {
        std::string topic = reader.read_string();
        uint8_t qos = reader.read_byte();
        assert(qos < 3);
        // TODO use emplace_back
        subscriptions.push_back(Subscription{topic, qos});
    } while(!reader.empty());
}

std::vector<uint8_t> SubscribePacket::serialize() const {

    std::vector<uint8_t> packet_data;
    PacketDataWriter writer(packet_data);

    return packet_data;
}

SubackPacket::SubackPacket(uint8_t command, const std::vector<uint8_t> &packet_data) {
    assert(static_cast<PacketType>(command >> 4) == PacketType::Suback);
    assert((command & 0x0F) == 0);

    type = PacketType::Suback;

    header_flags = command & 0x0F;

    PacketDataReader reader(packet_data);

    packet_id = reader.read_uint16();

    do {
        ReturnCode return_code = static_cast<ReturnCode>(reader.read_byte());
        return_codes.push_back(return_code);
    } while(!reader.empty());
}

std::vector<uint8_t> SubackPacket::serialize() const {

    std::vector<uint8_t> packet_data;
    PacketDataWriter writer(packet_data);
    writer.write_byte((static_cast<uint8_t>(type) << 4) | (header_flags & 0x0F));
    writer.write_remaining_length(2 + return_codes.size());
    for (auto return_code : return_codes) {
        writer.write_byte(static_cast<uint8_t>(return_code));
    }
    return packet_data;
}

UnsubscribePacket::UnsubscribePacket(uint8_t command, const std::vector<uint8_t> &packet_data) {
    assert(static_cast<PacketType>(command >> 4) == PacketType::Unsubscribe);
    assert((command & 0x0F) == 0x02);

    type = PacketType::Unsubscribe;

    header_flags = command & 0x0F;

    PacketDataReader reader(packet_data);

    packet_id = reader.read_uint16();

    do {
        topics.push_back(reader.read_string());
    } while(!reader.empty());
}

std::vector<uint8_t> UnsubscribePacket::serialize() const {

    std::vector<uint8_t> packet_data;
    PacketDataWriter writer(packet_data);
    writer.write_byte((static_cast<uint8_t>(type) << 4) | (header_flags & 0x0F));
    size_t topics_size = 0;
    for (auto topic : topics) {
        // string data + 2 byte length
        topics_size += topic.size() + 2;
    }
    writer.write_remaining_length(2 + topics_size);
    writer.write_uint16(packet_id);
    for (auto topic : topics) {
        writer.write_string(topic);
    }
    return packet_data;
}

UnsubackPacket::UnsubackPacket(uint8_t command, const std::vector<uint8_t> &packet_data) {
    assert(static_cast<PacketType>(command >> 4) == PacketType::Unsuback);
    assert((command & 0x0F) == 0x02);

    type = PacketType::Unsuback;

    header_flags = command & 0x0F;

    PacketDataReader reader(packet_data);

    packet_id = reader.read_uint16();
}

std::vector<uint8_t> UnsubackPacket::serialize() const {

    std::vector<uint8_t> packet_data;
    PacketDataWriter writer(packet_data);
    writer.write_byte((static_cast<uint8_t>(type) << 4) | (header_flags & 0x0F));
    writer.write_remaining_length(2);
    writer.write_uint16(packet_id);
    return packet_data;
}

PingreqPacket::PingreqPacket(uint8_t command, const std::vector<uint8_t> &packet_data) {

    assert(static_cast<PacketType>(command >> 4) == PacketType::Pingreq);
    assert(packet_data.size() == 0);

    type = PacketType::Pingreq;
    header_flags = command & 0x0F;

}

std::vector<uint8_t> PingreqPacket::serialize() const {
    std::vector<uint8_t> packet_data;
    PacketDataWriter writer(packet_data);
    writer.write_byte((static_cast<uint8_t>(type) << 4) | (header_flags & 0x0F));
    writer.write_remaining_length(0);
    return packet_data;
}

PingrespPacket::PingrespPacket(uint8_t command, const std::vector<uint8_t> &packet_data) {

    assert(static_cast<PacketType>(command >> 4) == PacketType::Pingresp);
    assert(packet_data.size() == 0);

    type = PacketType::Pingreq;
    header_flags = command & 0x0F;

}

std::vector<uint8_t> PingrespPacket::serialize() const {
    std::vector<uint8_t> packet_data;
    PacketDataWriter writer(packet_data);
    writer.write_byte((static_cast<uint8_t>(type) << 4) | (header_flags & 0x0F));
    writer.write_remaining_length(0);
    return packet_data;
}

DisconnectPacket::DisconnectPacket(uint8_t command, const std::vector<uint8_t> &packet_data) {

    assert(static_cast<PacketType>(command >> 4) == PacketType::Disconnect);
    assert(packet_data.size() == 0);

    type = PacketType::Disconnect;
    header_flags = command & 0x0F;

}

std::vector<uint8_t> DisconnectPacket::serialize() const {
    std::vector<uint8_t> packet_data;
    PacketDataWriter writer(packet_data);
    writer.write_byte((static_cast<uint8_t>(type) << 4) | (header_flags & 0x0F));
    writer.write_remaining_length(0);
    return packet_data;
}