#pragma once

#include "packet.h"
#include "packet_manager.h"

#include <event2/bufferevent.h>

class Session {

public:

    Session(struct bufferevent *bev, struct event_base *evbase) : packet_manager(bev, evbase) {
        packet_manager.set_packet_received_handler(std::bind(&Session::packet_received, this, std::placeholders::_1));
    }

    ~Session();

    void packet_received(const Packet &);

    void handle_connect(const ConnectPacket &);

    void handle_publish(const PublishPacket &);

    void handle_pubrel(const PubrelPacket &);

    void handle_subscribe(const SubscribePacket &);

    void handle_disconnect(const DisconnectPacket &);

    PacketManager packet_manager;
};

