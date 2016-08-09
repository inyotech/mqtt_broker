#pragma once

#include "packet_manager.h"
#include "packet.h"

#include <vector>
#include <memory>

#include <event2/bufferevent.h>

class SessionManager;

class Message;

class Subscription;

class Session {

public:

    Session(struct bufferevent *bev, SessionManager &session_manager) : packet_manager(new PacketManager(bev)),
                                                                        session_manager(session_manager) {
        packet_manager->set_packet_received_handler(std::bind(&Session::packet_received, this, std::placeholders::_1));
    }

    ~Session();

    void restore_session(Session *session, std::unique_ptr<PacketManager> packet_manager);

    void forward_packet(const PublishPacket &packet);

    void packet_received(std::unique_ptr<Packet>);

    void handle_connect(const ConnectPacket &);

    void handle_publish(const PublishPacket &);

    void handle_puback(const PubackPacket &);

    void handle_pubrec(const PubrecPacket &);

    void handle_pubrel(const PubrelPacket &);

    void handle_pubcomp(const PubcompPacket &);

    void handle_subscribe(const SubscribePacket &);

    void handle_unsubscribe(const UnsubscribePacket &);

    void handle_pingreq(const PingreqPacket &);

    void handle_disconnect(const DisconnectPacket &);

    uint16_t next_packet_id() {
        if (packet_id == 0) {
            packet_id++;
        }
        return packet_id;
    }

    uint16_t packet_id = 1;

    std::string client_id;

    std::unique_ptr<PacketManager> packet_manager;

    SessionManager &session_manager;

    std::vector<Subscription> subscriptions;

    // qos1 messages waiting for puback, will be moved between
    // sessions as part of session state
    std::vector<PublishPacket> qos1_sent;

    // qos2 messages waiting for pubrec, will be moved between
    // sessions as part of session state
    std::vector<PublishPacket> qos2_sent;

    // qos2 messages waiting for pubrel, will be moved between
    // sessions as part of session state
    std::vector<uint16_t> qos2_received;

    // qos2 pubrel packet ids waiting for pubcomp, will be moved
    // between sessions as part of session state
    std::vector<uint16_t> qos2_pubrel;
};

