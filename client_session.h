#pragma once

#include "packet_manager.h"
#include "packet.h"

#include <list>
#include <memory>

#include <event2/bufferevent.h>

class Subscription;

class ClientSession {

public:

    ClientSession(struct bufferevent *bev) : packet_manager(new PacketManager(bev)) {
        packet_manager->set_packet_received_handler(
                std::bind(&ClientSession::packet_received, this, std::placeholders::_1));
    }

    ~ClientSession();

    void packet_received(std::unique_ptr<Packet>);

    void send_connect(const ConnectPacket &);

    void send_publish(const PublishPacket &);

    void send_subscribe(const SubscribePacket &);

    void send_unsubscribe(const UnsubackPacket &);

    void send_disconnect(const DisconnectPacket &);

    void handle_connack(const ConnackPacket &);

    void handle_publish(const PublishPacket &);

    void handle_puback(const PubackPacket &);

    void handle_pubrec(const PubrecPacket &);

    void handle_pubrel(const PubrelPacket &);

    void handle_pubcomp(const PubcompPacket &);

    void handle_unsuback(const UnsubackPacket &);

    void handle_pingresp(const PingrespPacket &);

    uint16_t next_packet_id() {
        if (packet_id == 0) {
            packet_id++;
        }
        return packet_id;
    }

    uint16_t packet_id = 1;

    std::string client_id;

    std::unique_ptr<PacketManager> packet_manager;

    std::vector<Subscription> subscriptions;
};

