#pragma once

#include "packet_data.h"
#include "topic.h"

#include <iostream>
#include <string>
#include <cstdint>
#include <vector>
#include <functional>
#include <memory>
#include <cassert>

#include <event2/event.h>
#include <event2/bufferevent.h>

enum class PacketType {
    Connect = 1,
    Connack = 2,
    Publish = 3,
    Puback = 4,
    Pubrec = 5,
    Pubrel = 6,
    Pubcomp = 7,
    Subscribe = 8,
    Suback = 9,
    Unsubscribe = 10,
    Unsuback = 11,
    Pingreq = 12,
    Pingresp = 13,
    Disconnect = 14,
};

enum class QoSType : uint8_t {
    QoS0 = 0,
    QoS1 = 1,
    QoS2 = 2,
};

class Subscription {
public:
    TopicFilter topic_filter;
    QoSType qos;
};

class Packet {
public:
    PacketType type;
    uint8_t header_flags;

    virtual ~Packet() {}

    void read_fixed_header(PacketDataReader &);

    //  static Packet unserialize(const std::vector<uint8_t> &);
    virtual std::vector<uint8_t> serialize(void) const = 0;

};

class ConnectPacket : public Packet {
public:

    ConnectPacket() {
        type = PacketType::Connect;
        header_flags = 0;
        protocol_name = "MQIsdp";
        protocol_level = 4;
    }

    ConnectPacket(const std::vector<uint8_t> & packet_data);

    std::vector<uint8_t> serialize() const;

    std::string protocol_name;
    uint8_t protocol_level;
    uint8_t connect_flags;
    uint16_t keep_alive;
    std::string client_id;
    std::string will_topic;
    std::vector<uint8_t> will_message;
    std::string username;
    std::vector<uint8_t> password;

    bool clean_session() const {
        return connect_flags & 0x02;
    }

    void clean_session(bool set) {
        if (set) {
            connect_flags |= 0x02;
        } else {
            connect_flags &= ~0x02;
        }
    }

    bool will_flag() const {
        return connect_flags & 0x04;
    }

    void will_flag(bool set) {
        if (set) {
            connect_flags |= 0x04;
        } else {
            connect_flags &= ~0x04;
        }
    }

    QoSType qos() const {
        return static_cast<QoSType >((connect_flags >> 3) & 0x03);
    }

    void qos(QoSType qos) {
        connect_flags |= (static_cast<uint8_t>(qos) << 3);
    }

    bool will_retain() const {
        return connect_flags & 0x20;
    }

    void will_retain(bool set) {
        if (set) {
            connect_flags |= 0x20;
        } else {
            connect_flags &= ~0x20;
        }
    }

    bool password_flag() const {
        return connect_flags & 0x40;
    }

    void password_flag(bool set) {
        if (set) {
            connect_flags |= 0x40;
        } else {
            connect_flags &= ~0x40;
        }
    }

    bool username_flag() const {
        return connect_flags & 0x80;
    }

    void username_flag(bool set) {
        if (set) {
            connect_flags |= 0x80;
        } else {
            connect_flags &= ~0x80;
        }
    }

};

class ConnackPacket : public Packet {
public:

    ConnackPacket() {
        type = PacketType::Connack;
        header_flags = 0;
    }

    ConnackPacket(const std::vector<uint8_t> &packet_data);

    enum class ReturnCode : uint8_t {
        Accepted = 0x00,
        UnacceptableProtocolVersion = 0x01,
        IdentifierRejected = 0x02,
        ServerUnavailable = 0x03,
        BadUsernameOrPassword = 0x04,
        NotAuthorized = 0x05
    };

    std::vector<uint8_t> serialize() const;

    uint8_t acknowledge_flags;
    ReturnCode return_code;

    bool session_present() const {
        return acknowledge_flags & 0x01;
    }

    void session_present(bool set) {
        if (set) {
            acknowledge_flags |= 0x01;
        } else {
            acknowledge_flags &= ~0x01;
        }
    }

};

class PublishPacket : public Packet {
public:

    PublishPacket() {
        type = PacketType::Publish;
        header_flags = 0;
    }

    PublishPacket(const std::vector<uint8_t> &packet_data);

    std::vector<uint8_t> serialize() const;

    std::string topic_name;
    std::vector<uint8_t> message_data;
    uint16_t packet_id;

    bool dup() const {
        return header_flags & 0x08;
    }

    void dup(bool set) {
        if (set) {
            header_flags |= 0x08;
        } else {
            header_flags &= ~0x08;
        }
    }

    QoSType qos() const {
        return static_cast<QoSType >((header_flags >> 1) & 0x03);
    }

    void qos(QoSType qos) {
        header_flags |= static_cast<uint8_t>(qos) << 1;
    }

    bool retain() const {
        return header_flags & 0x01;
    }

    void retain(bool set) {
        if (set) {
            header_flags |= 0x01;
        } else {
            header_flags &= ~0x01;
        }
    }
};

class PubackPacket : public Packet {

public:

    PubackPacket() {
        type = PacketType::Puback;
        header_flags = 0;
    }

    PubackPacket(const std::vector<uint8_t> &packet_data);

    std::vector<uint8_t> serialize() const;

    uint16_t packet_id;
};

class PubrecPacket : public Packet {

public:

    PubrecPacket() {
        type = PacketType::Pubrec;
        header_flags = 0;
    }

    PubrecPacket(const std::vector<uint8_t> &packet_data);

    std::vector<uint8_t> serialize() const;

    uint16_t packet_id;
};

class PubrelPacket : public Packet {

public:

    PubrelPacket() {
        type = PacketType::Pubrel;
        header_flags = 0x02;
    }

    PubrelPacket(const std::vector<uint8_t> &packet_data);

    std::vector<uint8_t> serialize() const;

    uint16_t packet_id;
};

class PubcompPacket : public Packet {

public:

    PubcompPacket() {
        type = PacketType::Pubcomp;
        header_flags = 0;
    }

    PubcompPacket(const std::vector<uint8_t> &packet_data);

    std::vector<uint8_t> serialize() const;

    uint16_t packet_id;
};

class SubscribePacket : public Packet {

public:

    SubscribePacket() {
        type = PacketType::Subscribe;
        header_flags = 0x02;
    }

    SubscribePacket(const std::vector<uint8_t> &packet_data);

    std::vector<uint8_t> serialize() const;

    uint16_t packet_id;

    std::vector<Subscription> subscriptions;

};

class SubackPacket : public Packet {

public:

    SubackPacket() {
        type = PacketType::Suback;
        header_flags = 0;
    }

    SubackPacket(const std::vector<uint8_t> &packet_data);

    std::vector<uint8_t> serialize() const;

    enum class ReturnCode : uint8_t {
        SuccessQoS0 = 0x00,
        SuccessQoS1 = 0x01,
        SuccessQoS2 = 0x02,
        Failure = 0x80
    };

    uint16_t packet_id;
    std::vector<ReturnCode> return_codes;

};

class UnsubscribePacket : public Packet {

public:

    UnsubscribePacket() {
        type = PacketType::Unsubscribe;
        header_flags = 0x02;
    }

    UnsubscribePacket(const std::vector<uint8_t> &packet_data);

    std::vector<uint8_t> serialize() const;

    uint16_t packet_id;

    std::vector<std::string> topics;

};

class UnsubackPacket : public Packet {

public:

    UnsubackPacket() {
        type = PacketType::Unsuback;
        header_flags = 0;
    }

    UnsubackPacket(const std::vector<uint8_t> &packet_data);

    std::vector<uint8_t> serialize() const;

    uint16_t packet_id;

};

class PingreqPacket : public Packet {

public:

    PingreqPacket() {
        type = PacketType::Pingreq;
        header_flags = 0;
    }

    PingreqPacket(const std::vector<uint8_t> &packet_data);

    std::vector<uint8_t> serialize() const;
};

class PingrespPacket : public Packet {

public:

    PingrespPacket() {
        type = PacketType::Pingresp;
        header_flags = 0;
    }

    PingrespPacket(const std::vector<uint8_t> &packet_data);

    std::vector<uint8_t> serialize() const;
};

class DisconnectPacket : public Packet {

public:

    DisconnectPacket() {
        type = PacketType::Disconnect;
        header_flags = 0;
    }

    DisconnectPacket(const std::vector<uint8_t> &packet_data);

    std::vector<uint8_t> serialize() const;
};

