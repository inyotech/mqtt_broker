//
// Created by Scott Brumbaugh on 8/3/16.
//

#pragma once

#include "packet.h"

#include <iostream>
#include <vector>
#include <cstddef>
#include <memory>

#include <event2/event.h>
#include <event2/bufferevent.h>

class PacketManager {
public:

    enum class EventType {
        NetworkError,
        ProtocolError,
        ConnectionClosed,
        Timeout,
    };

    PacketManager(struct bufferevent * bev) : bev(bev) {
        bufferevent_setcb(bev, input_ready, NULL, other_event, this);
        bufferevent_enable(bev, EV_READ);
    }

    virtual ~PacketManager() {
        if (bev) {
            bufferevent_free(bev);
            bev = nullptr;
        }
    }

    static void input_ready(struct bufferevent * bev, void * arg) {
        PacketManager * _this = static_cast<PacketManager *>(arg);
        _this->receive_packet_data(bev);
    }

    static void other_event(struct bufferevent * bev, short events, void * arg) {
        PacketManager * _this = static_cast<PacketManager *>(arg);
        _this->handle_other_events(events);
    }

    void receive_packet_data(struct bufferevent * bev);

    std::unique_ptr<Packet> parse_packet_data(const packet_data_t & packet_data);

    void handle_other_events(short events);

    void set_packet_received_handler(std::function<void(std::unique_ptr<Packet>)> handler) {
        packet_received_handler = handler;
    }

    void set_event_handler(std::function<void(EventType)> handler) {
        event_handler = handler;
    }

    void close_connection();

    uint16_t next_packet_id() {

        if (++packet_id == 0) {
            ++packet_id;
        }
        return packet_id;
    }

    uint16_t packet_id = 0;

    void send_packet(const Packet &);

    size_t fixed_header_length = 0;
    size_t remaining_length = 0;

    struct bufferevent * bev;

    std::function<void(std::unique_ptr<Packet>)> packet_received_handler;

    std::function<void(EventType)> event_handler;
};