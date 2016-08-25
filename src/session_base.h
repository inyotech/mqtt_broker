//
// Created by Scott Brumbaugh on 8/23/16.
//

#pragma once

#include "packet.h"
#include "packet_manager.h"

#include <event2/bufferevent.h>
#include <string>

class SessionBase {

public:

    SessionBase(struct bufferevent *bev) : packet_manager(new PacketManager(bev)) {
        packet_manager->set_packet_received_handler(
                std::bind(&SessionBase::packet_received, this, std::placeholders::_1));
        packet_manager->set_event_handler(std::bind(&SessionBase::packet_manager_event, this, std::placeholders::_1));
    }

    virtual ~SessionBase() {}

    std::string client_id;

    virtual void packet_received(std::unique_ptr<Packet>);

    virtual void packet_manager_event(PacketManager::EventType event);

    virtual void handle_connect(const ConnectPacket &);

    virtual void handle_connack(const ConnackPacket &);

    virtual void handle_publish(const PublishPacket &);

    virtual void handle_puback(const PubackPacket &);

    virtual void handle_pubrec(const PubrecPacket &);

    virtual void handle_pubrel(const PubrelPacket &);

    virtual void handle_pubcomp(const PubcompPacket &);

    virtual void handle_subscribe(const SubscribePacket &);

    virtual void handle_suback(const SubackPacket &);

    virtual void handle_unsubscribe(const UnsubscribePacket &);

    virtual void handle_unsuback(const UnsubackPacket &);

    virtual void handle_pingreq(const PingreqPacket &);

    virtual void handle_pingresp(const PingrespPacket &);

    virtual void handle_disconnect(const DisconnectPacket &);

    std::unique_ptr<PacketManager> packet_manager;

};