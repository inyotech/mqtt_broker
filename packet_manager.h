//
// Created by Scott Brumbaugh on 8/3/16.
//

#pragma once

#include "packet.h"

#include <cstddef>

class PacketManager {
public:

    PacketManager(struct bufferevent * bev, struct event_base * evbase) : bev(bev), evbase(evbase) {
        bufferevent_setcb(bev, input_ready, NULL, other_event, this);
        bufferevent_enable(bev, EV_READ|EV_WRITE);
    }

    ~PacketManager() {
        std::cout << "deleting packet_manager\n";
        bufferevent_free(bev);
    }

    static void input_ready(struct bufferevent * bev, void * _this) {
        PacketManager * p = static_cast<PacketManager *>(_this);
        p->receive_packet_data(bev);
    }

    static void other_event(struct bufferevent * bev, short events, void * _this) {
        PacketManager * p = static_cast<PacketManager *>(_this);
        p->handle_other_events(events);
    }

    void receive_packet_data(struct bufferevent * bev);

    std::unique_ptr<Packet> parse_packet_data(uint8_t packet_type);

    void handle_other_events(short events);

    void set_packet_received_handler(std::function<void(const Packet &)> handler) {
        packet_received_handler = handler;
    }

    void send_packet(const Packet &);

    size_t fixed_header_length = 0;
    size_t remaining_length = 0;

    std::vector<uint8_t> packet_data_in;

    struct bufferevent * bev;
    struct event_base * evbase;

    std::function<void(const Packet &)> packet_received_handler;

};