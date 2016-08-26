/**
 * @file packet.cc
 */

#include "packet.h"
#include "client_id.h"

#include <cassert>

void Packet::read_fixed_header(PacketDataReader &reader) {

    uint8_t command_header = reader.read_byte();
    type = static_cast<PacketType>(command_header >> 4);
    header_flags = command_header & 0x0F;

    size_t remaining_length = reader.read_remaining_length();
    if (remaining_length != reader.get_packet_data().size() - reader.get_offset()) {
        throw std::exception();
    }
}

ConnectPacket::ConnectPacket(const packet_data_t &packet_data) {

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

packet_data_t ConnectPacket::serialize() const {
    packet_data_t packet_data;
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

ConnackPacket::ConnackPacket(const packet_data_t &packet_data) {

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

packet_data_t ConnackPacket::serialize() const {

    packet_data_t packet_data;
    PacketDataWriter writer(packet_data);

    writer.write_byte((static_cast<uint8_t>(type) << 4) | (header_flags & 0x0F));
    writer.write_remaining_length(2);
    writer.write_byte(acknowledge_flags);
    writer.write_byte(static_cast<uint8_t>(return_code));
    return packet_data;
}

PublishPacket::PublishPacket(const packet_data_t &packet_data) {

    PacketDataReader reader(packet_data);

    read_fixed_header(reader);

    if (type != PacketType::Publish) {
        throw std::exception();
    }

    topic_name = reader.read_string();

    if (qos() != QoSType::QoS0) {
        packet_id = reader.read_uint16();
    }

    size_t payload_len = packet_data.size() - reader.get_offset();

    message_data = reader.read_bytes(payload_len);

}

packet_data_t PublishPacket::serialize() const {
    packet_data_t packet_data;
    PacketDataWriter writer(packet_data);
    writer.write_byte((static_cast<uint8_t>(type) << 4) | (header_flags & 0x0F));
    uint16_t remaining_length = 2 + topic_name.size() + message_data.size();
    if (qos() != QoSType::QoS0) {
        remaining_length += 2;
    }
    writer.write_remaining_length(remaining_length);
    writer.write_string(topic_name);
    if (qos() != QoSType::QoS0) {
        writer.write_uint16(packet_id);
    }
    for (size_t i = 0; i < message_data.size(); i++) {
        writer.write_byte(message_data[i]);
    }

    return packet_data;
}

PubackPacket::PubackPacket(const packet_data_t &packet_data) {

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

packet_data_t PubackPacket::serialize() const {

    packet_data_t packet_data;
    PacketDataWriter writer(packet_data);
    writer.write_byte((static_cast<uint8_t>(type) << 4) | (header_flags & 0x0F));
    writer.write_remaining_length(2);
    writer.write_uint16(packet_id);
    return packet_data;
}

PubrecPacket::PubrecPacket(const packet_data_t &packet_data) {

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

packet_data_t PubrecPacket::serialize() const {

    packet_data_t packet_data;
    PacketDataWriter writer(packet_data);
    writer.write_byte((static_cast<uint8_t>(type) << 4) | (header_flags & 0x0F));
    writer.write_remaining_length(2);
    writer.write_uint16(packet_id);
    return packet_data;
}

PubrelPacket::PubrelPacket(const packet_data_t &packet_data) {

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

packet_data_t PubrelPacket::serialize() const {

    packet_data_t packet_data;
    PacketDataWriter writer(packet_data);
    writer.write_byte((static_cast<uint8_t>(type) << 4) | (header_flags & 0x0F));
    writer.write_remaining_length(2);
    writer.write_uint16(packet_id);
    return packet_data;
}

PubcompPacket::PubcompPacket(const packet_data_t &packet_data) {

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

packet_data_t PubcompPacket::serialize() const {

    packet_data_t packet_data;
    PacketDataWriter writer(packet_data);
    writer.write_byte((static_cast<uint8_t>(type) << 4) | (header_flags & 0x0F));
    writer.write_remaining_length(2);
    writer.write_uint16(packet_id);
    return packet_data;
}

SubscribePacket::SubscribePacket(const packet_data_t &packet_data) {

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
        QoSType qos = static_cast<QoSType>(reader.read_byte());
        // TODO use emplace_back
        subscriptions.push_back(Subscription{topic, qos});
    } while (!reader.empty());
}

packet_data_t SubscribePacket::serialize() const {

    packet_data_t packet_data;
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
        writer.write_byte(static_cast<uint8_t>(subscription.qos));
    }

    return packet_data;

}

SubackPacket::SubackPacket(const packet_data_t &packet_data) {

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

packet_data_t SubackPacket::serialize() const {

    packet_data_t packet_data;
    PacketDataWriter writer(packet_data);
    writer.write_byte((static_cast<uint8_t>(type) << 4) | (header_flags & 0x0F));
    writer.write_remaining_length(2 + return_codes.size());
    writer.write_uint16(packet_id);
    for (auto return_code : return_codes) {
        writer.write_byte(static_cast<uint8_t>(return_code));
    }
    return packet_data;
}

UnsubscribePacket::UnsubscribePacket(const packet_data_t &packet_data) {

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

packet_data_t UnsubscribePacket::serialize() const {

    packet_data_t packet_data;
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

UnsubackPacket::UnsubackPacket(const packet_data_t &packet_data) {

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

packet_data_t UnsubackPacket::serialize() const {

    packet_data_t packet_data;
    PacketDataWriter writer(packet_data);
    writer.write_byte((static_cast<uint8_t>(type) << 4) | (header_flags & 0x0F));
    writer.write_remaining_length(2);
    writer.write_uint16(packet_id);
    return packet_data;
}

PingreqPacket::PingreqPacket(const packet_data_t &packet_data) {

    PacketDataReader reader(packet_data);

    read_fixed_header(reader);

    if (type != PacketType::Pingreq) {
        throw std::exception();
    }

    if (header_flags != 0) {
        throw std::exception();
    }
}

packet_data_t PingreqPacket::serialize() const {

    packet_data_t packet_data;
    PacketDataWriter writer(packet_data);
    writer.write_byte((static_cast<uint8_t>(type) << 4) | (header_flags & 0x0F));
    writer.write_remaining_length(0);
    return packet_data;
}

PingrespPacket::PingrespPacket(const packet_data_t &packet_data) {

    PacketDataReader reader(packet_data);

    read_fixed_header(reader);

    if (type != PacketType::Pingresp) {
        throw std::exception();
    }

    if (header_flags != 0) {
        throw std::exception();
    }
}

packet_data_t PingrespPacket::serialize() const {
    packet_data_t packet_data;
    PacketDataWriter writer(packet_data);
    writer.write_byte((static_cast<uint8_t>(type) << 4) | (header_flags & 0x0F));
    writer.write_remaining_length(0);
    return packet_data;
}

DisconnectPacket::DisconnectPacket(const packet_data_t &packet_data) {

    PacketDataReader reader(packet_data);

    read_fixed_header(reader);

    if (type != PacketType::Disconnect) {
        throw std::exception();
    }

    if (header_flags != 0) {
        throw std::exception();
    }

}

packet_data_t DisconnectPacket::serialize() const {
    packet_data_t packet_data;
    PacketDataWriter writer(packet_data);
    writer.write_byte((static_cast<uint8_t>(type) << 4) | (header_flags & 0x0F));
    writer.write_remaining_length(0);
    return packet_data;
}
