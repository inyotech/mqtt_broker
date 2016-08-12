//
// Created by Scott Brumbaugh on 8/3/16.
//

#pragma once

#include "mqtt.h"

#include <iostream>
#include <vector>
#include <cstddef>
#include <memory>

#include <event2/event.h>
#include <event2/bufferevent.h>

class PacketManager {
public:

    PacketManager(struct bufferevent * bev) : bev(bev) {
        bufferevent_setcb(bev, input_ready, NULL, other_event, this);
        bufferevent_enable(bev, EV_READ);
    }

    ~PacketManager() {
        std::cout << "~PacketManager\n";
        if (bev) {
            bufferevent_free(bev);
            bev = nullptr;
        }
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

    owned_packet_ptr_t parse_packet_data(const packet_data_t & packet_data);

    void handle_other_events(short events);

    void set_packet_received_handler(std::function<void(owned_packet_ptr_t)> handler) {
        packet_received_handler = handler;
    }

    void send_packet(const Packet &);

    size_t fixed_header_length = 0;
    size_t remaining_length = 0;

    struct bufferevent * bev;

    std::function<void(owned_packet_ptr_t)> packet_received_handler;

};