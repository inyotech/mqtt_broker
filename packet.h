#pragma once

#include "packet_data.h"

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
    Disconnect = 14
};

class Packet {
public:
    PacketType type;
    uint8_t header_flags;

    virtual ~Packet() {}

    //  static Packet unserialize(const std::vector<uint8_t> &);
    virtual std::vector<uint8_t> serialize(void) const = 0;

};

class ConnectPacket : public Packet {
public:

    ConnectPacket() {
        type = PacketType::Connect;
    }

    ConnectPacket(uint8_t command, const std::vector<uint8_t> &packet_data);

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

    uint8_t qos() const {
        return (connect_flags >> 3) & 0x03;
    }

    void qos(uint8_t qos) {
        assert(qos < 3);
        connect_flags |= (qos << 3);
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

    ConnackPacket(uint8_t command, const std::vector<uint8_t> &packet_data);

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
    }

    PublishPacket(uint8_t command, const std::vector<uint8_t> &packet_data);

    std::vector<uint8_t> serialize() const;

    std::string topic_name;
    uint16_t packet_id;
    std::vector<uint8_t> message;

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

    uint8_t qos() const {
        return (header_flags >> 1) & 0x03;
    }

    void qos(uint8_t qos) {
        assert(qos < 3);
        header_flags |= (qos << 1);
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

    PubackPacket(uint8_t command, const std::vector<uint8_t> &packet_data);

    std::vector<uint8_t> serialize() const;

    uint16_t packet_id;
};

class PubrecPacket : public Packet {

public:

    PubrecPacket() {
        type = PacketType::Pubrec;
        header_flags = 0;
    }

    PubrecPacket(uint8_t command, const std::vector<uint8_t> &packet_data);

    std::vector<uint8_t> serialize() const;

    uint16_t packet_id;
};

class PubrelPacket : public Packet {

public:

    PubrelPacket() {
        type = PacketType::Pubrel;
        header_flags = 0x02;
    }

    PubrelPacket(uint8_t command, const std::vector<uint8_t> &packet_data);

    std::vector<uint8_t> serialize() const;

    uint16_t packet_id;
};

class PubcompPacket : public Packet {

public:

    PubcompPacket() {
        type = PacketType::Pubcomp;
        header_flags = 0;
    }

    PubcompPacket(uint8_t command, const std::vector<uint8_t> &packet_data);

    std::vector<uint8_t> serialize() const;

    uint16_t packet_id;
};

class SubscribePacket : public Packet {

public:

    SubscribePacket() {
        type = PacketType::Subscribe;
        header_flags = 0x02;
    }

    SubscribePacket(uint8_t command, const std::vector<uint8_t> &packet_data);

    std::vector<uint8_t> serialize() const;

    uint16_t packet_id;

    struct Subscription {
        std::string topic;
        uint8_t qos;
    };

    std::vector<Subscription> subscriptions;

};

class DisconnectPacket : public Packet {

public:

    DisconnectPacket() {
        type = PacketType::Disconnect;
        header_flags = 0;
    }

    DisconnectPacket(uint8_t command, const std::vector<uint8_t> &packet_data);

    std::vector<uint8_t> serialize() const;
};

