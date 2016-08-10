#include "packet.h"
#include "client_id.h"

#include <cstring>
#include <iostream>
#include <cassert>

#include <event2/buffer.h>

void Packet::read_fixed_header(PacketDataReader & reader) {

    uint8_t command_header = reader.read_byte();
    type = static_cast<PacketType>(command_header >> 4);
    header_flags = command_header & 0x0F;

    size_t remaining_length = reader.read_remaining_length();
    if (remaining_length != reader.get_packet_data().size() - reader.get_offset()) {
        throw std::exception();
    }
}

ConnectPacket::ConnectPacket(const std::vector<uint8_t> &packet_data) {

    PacketDataReader reader(packet_data);

    read_fixed_header(reader);

    if (type != PacketType::Connect) {
        throw std::exception();
    }

    if (header_flags != 0) {
        throw std::exception();
    }

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
    writer.write_byte((static_cast<uint8_t>(type) << 4) | (header_flags & 0x0F));

    size_t remaining_length = 2 + protocol_name.size();
    remaining_length += 1; // protocol_level
    remaining_length += 1; // connect_flags
    remaining_length += 2; // keep_alive
    remaining_length += 2 + client_id.size();

    if (will_flag()) {
        remaining_length += 2 + will_topic.size();
        remaining_length += 2 + will_message.size();
    }

    if (username_flag()) {
        remaining_length += 2 + username.size();
    }

    if (password_flag()) {
        remaining_length += 2 + password.size();
    }

    writer.write_remaining_length(remaining_length);
    writer.write_string(protocol_name);
    writer.write_byte(protocol_level);
    writer.write_byte(connect_flags);
    writer.write_uint16(keep_alive);
    writer.write_string(client_id);

    if (will_flag()) {
        writer.write_string(will_topic);
        writer.write_bytes(will_message);
    }

    if (username_flag()) {
        writer.write_string(username);
    }

    if (password_flag()) {
        writer.write_bytes(password);
    }

    return packet_data;
}

ConnackPacket::ConnackPacket(const std::vector<uint8_t> & packet_data) {

    PacketDataReader reader(packet_data);

    read_fixed_header(reader);

    if (type != PacketType::Connack) {
        throw std::exception();
    }

    if (header_flags != 0) {
        throw std::exception();
    }

    acknowledge_flags = reader.read_byte();
    return_code = static_cast<ReturnCode>(reader.read_byte());
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

PublishPacket::PublishPacket(const std::vector<uint8_t> &packet_data) {

    PacketDataReader reader(packet_data);

    read_fixed_header(reader);

    if (type != PacketType::Publish) {
        throw std::exception();
    }

    topic_name = reader.read_string();

    if (qos() != 0) {
        packet_id = reader.read_uint16();
    }

    size_t header_len = topic_name.size() + 2;
    if (qos() != 0) {
        header_len += 2;
    }

    size_t payload_len = packet_data.size() - reader.get_offset();

    message_data = reader.read_bytes(payload_len);

}

std::vector<uint8_t> PublishPacket::serialize() const {
    std::vector<uint8_t> packet_data;
    PacketDataWriter writer(packet_data);
    writer.write_byte((static_cast<uint8_t>(type) << 4) | (header_flags & 0x0F));
    uint16_t remaining_length = 2 + topic_name.size() + message_data.size();
    if (qos() != 0) {
        remaining_length += 2;
    }
    writer.write_remaining_length(remaining_length);
    writer.write_string(topic_name);
    if (qos() != 0) {
        writer.write_uint16(packet_id);
    }
    for (int i = 0; i < message_data.size(); i++) {
        writer.write_byte(message_data[i]);
    }

    return packet_data;
}

PubackPacket::PubackPacket(const std::vector<uint8_t> &packet_data) {

    PacketDataReader reader(packet_data);

    read_fixed_header(reader);

    if (type != PacketType::Puback) {
        throw std::exception();
    }

    if (header_flags != 0) {
        throw std::exception();
    }

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

PubrecPacket::PubrecPacket(const std::vector<uint8_t> &packet_data) {

    PacketDataReader reader(packet_data);

    read_fixed_header(reader);

    if (type != PacketType::Pubrec) {
        throw std::exception();
    }

    if (header_flags != 0) {
        throw std::exception();
    }

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

PubrelPacket::PubrelPacket(const std::vector<uint8_t> &packet_data) {

    PacketDataReader reader(packet_data);

    read_fixed_header(reader);

    if (type != PacketType::Pubrel) {
        throw std::exception();
    }

    if (header_flags != 0x02) {
        throw std::exception();
    }

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

PubcompPacket::PubcompPacket(const std::vector<uint8_t> &packet_data) {

    PacketDataReader reader(packet_data);

    read_fixed_header(reader);

    if (type != PacketType::Pubcomp) {
        throw std::exception();
    }

    if (header_flags != 0) {
        throw std::exception();
    }

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

SubscribePacket::SubscribePacket(const std::vector<uint8_t> &packet_data) {

    PacketDataReader reader(packet_data);

    read_fixed_header(reader);

    if (type != PacketType::Subscribe) {
        throw std::exception();
    }

    if (header_flags != 0x02) {
        throw std::exception();
    }

    packet_id = reader.read_uint16();

    do {
        std::string topic = reader.read_string();
        uint8_t qos = reader.read_byte();
        assert(qos < 3);
        // TODO use emplace_back
        subscriptions.push_back(Subscription{topic, qos});
    } while (!reader.empty());
}

std::vector<uint8_t> SubscribePacket::serialize() const {

    std::vector<uint8_t> packet_data;
    PacketDataWriter writer(packet_data);

    writer.write_byte((static_cast<uint8_t>(type) << 4) | (header_flags & 0x0F));

    size_t remaining_length = 2;
    for (auto subscription : subscriptions) {
        remaining_length += 1 + 2 + std::string(subscription.topic_filter).size();
    }

    writer.write_remaining_length(remaining_length);
    writer.write_uint16(packet_id);

    for (auto subscription : subscriptions) {
        writer.write_string(std::string(subscription.topic_filter));
        writer.write_byte(subscription.qos);
    }

    return packet_data;

}

SubackPacket::SubackPacket(const std::vector<uint8_t> &packet_data) {

    PacketDataReader reader(packet_data);

    read_fixed_header(reader);

    if (type != PacketType::Suback) {
        throw std::exception();
    }

    if (header_flags != 0) {
        throw std::exception();
    }

    packet_id = reader.read_uint16();

    do {
        ReturnCode return_code = static_cast<ReturnCode>(reader.read_byte());
        return_codes.push_back(return_code);
    } while (!reader.empty());
}

std::vector<uint8_t> SubackPacket::serialize() const {

    std::vector<uint8_t> packet_data;
    PacketDataWriter writer(packet_data);
    writer.write_byte((static_cast<uint8_t>(type) << 4) | (header_flags & 0x0F));
    writer.write_remaining_length(2 + return_codes.size());
    writer.write_uint16(packet_id);
    for (auto return_code : return_codes) {
        writer.write_byte(static_cast<uint8_t>(return_code));
    }
    return packet_data;
}

UnsubscribePacket::UnsubscribePacket(const std::vector<uint8_t> &packet_data) {

    PacketDataReader reader(packet_data);

    read_fixed_header(reader);

    if (type != PacketType::Unsubscribe) {
        throw std::exception();
    }

    if (header_flags != 0x02) {
        throw std::exception();
    }

    packet_id = reader.read_uint16();

    do {
        topics.push_back(reader.read_string());
    } while (!reader.empty());
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

UnsubackPacket::UnsubackPacket(const std::vector<uint8_t> &packet_data) {

    PacketDataReader reader(packet_data);

    read_fixed_header(reader);

    if (type != PacketType::Unsuback) {
        throw std::exception();
    }

    if (header_flags != 0) {
        throw std::exception();
    }

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

PingreqPacket::PingreqPacket(const std::vector<uint8_t> &packet_data) {

    PacketDataReader reader(packet_data);

    read_fixed_header(reader);

    if (type != PacketType::Pingreq) {
        throw std::exception();
    }

    if (header_flags != 0) {
        throw std::exception();
    }
}

std::vector<uint8_t> PingreqPacket::serialize() const {

    std::vector<uint8_t> packet_data;
    PacketDataWriter writer(packet_data);
    writer.write_byte((static_cast<uint8_t>(type) << 4) | (header_flags & 0x0F));
    writer.write_remaining_length(0);
    return packet_data;
}

PingrespPacket::PingrespPacket(const std::vector<uint8_t> &packet_data) {

    PacketDataReader reader(packet_data);

    read_fixed_header(reader);

    if (type != PacketType::Pingresp) {
        throw std::exception();
    }

    if (header_flags != 0) {
        throw std::exception();
    }
}

std::vector<uint8_t> PingrespPacket::serialize() const {
    std::vector<uint8_t> packet_data;
    PacketDataWriter writer(packet_data);
    writer.write_byte((static_cast<uint8_t>(type) << 4) | (header_flags & 0x0F));
    writer.write_remaining_length(0);
    return packet_data;
}

DisconnectPacket::DisconnectPacket(const std::vector<uint8_t> &packet_data) {

    PacketDataReader reader(packet_data);

    read_fixed_header(reader);

    if (type != PacketType::Disconnect) {
        throw std::exception();
    }

    if (header_flags != 0) {
        throw std::exception();
    }

}

std::vector<uint8_t> DisconnectPacket::serialize() const {
    std::vector<uint8_t> packet_data;
    PacketDataWriter writer(packet_data);
    writer.write_byte((static_cast<uint8_t>(type) << 4) | (header_flags & 0x0F));
    writer.write_remaining_length(0);
    return packet_data;
}