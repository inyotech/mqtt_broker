//
// Created by Scott Brumbaugh on 8/11/16.
//

#include <iostream>
#include <vector>

#include <getopt.h>

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/dns.h>
#include <evdns.h>

#include "mqtt.h"
#include "client_session.h"

static void usage(void);

static void parse_arguments(int argc, char *argv[]);

static void connect_event_cb(struct bufferevent *, short event, void *);

static void close_cb(struct bufferevent *bev, void *arg);

static void packet_received_callback(owned_packet_ptr_t Packet);

static uint16_t next_packet_id(void);

std::string broker_host = "localhost";
uint16_t broker_port = 1883;
std::string client_id;
std::string topic;
std::vector<uint8_t> message;
uint8_t qos = 0;
bool clean_session = false;

PacketManager *packet_manager;

int main(int argc, char *argv[]) {

    struct event_base *evloop;
    struct evdns_base *dns_base;
    struct bufferevent *bev;

    parse_arguments(argc, argv);

    evloop = event_base_new();
    if (!evloop) {
        std::cerr << "Could not initialize libevent\n";
        std::exit(1);
    }

    dns_base = evdns_base_new(evloop, 1);

    bev = bufferevent_socket_new(evloop, -1, BEV_OPT_CLOSE_ON_FREE);

    bufferevent_setcb(bev, NULL, NULL, connect_event_cb, evloop);

    bufferevent_socket_connect_hostname(bev, dns_base, AF_UNSPEC, broker_host.c_str(), broker_port);

    event_base_dispatch(evloop);
    event_base_free(evloop);

}

static void connect_event_cb(struct bufferevent *bev, short events, void *arg) {

    std::cout << "connect event\n";

    if (events & BEV_EVENT_CONNECTED) {
        std::cout << "Connect okay.\n";

        packet_manager = new PacketManager(bev);
        packet_manager->set_packet_received_handler(packet_received_callback);

        ConnectPacket connect_packet;
        connect_packet.client_id = client_id;
        packet_manager->send_packet(connect_packet);

    } else if (events & (BEV_EVENT_ERROR | BEV_EVENT_EOF)) {
        std::cout << "closing\n";
        struct event_base *base = static_cast<struct event_base *>(arg);
        if (events & BEV_EVENT_ERROR) {
            int err = bufferevent_socket_get_dns_error(bev);
            if (err)
                std::cerr << "DNS error: " << evutil_gai_strerror(err) << "\n";
        }

        bufferevent_free(bev);
        event_base_loopexit(base, NULL);
    }

}

void packet_received_callback(owned_packet_ptr_t packet_ptr) {

    switch (packet_ptr->type) {
        case PacketType::Connack: {

            PublishPacket publish_packet;
            publish_packet.qos(qos);
            publish_packet.topic_name = topic;
            publish_packet.packet_id = next_packet_id();
            publish_packet.message_data = std::vector<uint8_t>(message.begin(), message.end());
            packet_manager->send_packet(publish_packet);
            break;
        }

        case PacketType::Pubrec: {

            PubrelPacket pubrel_packet;
            pubrel_packet.packet_id = dynamic_cast<PubrecPacket &>(*packet_ptr).packet_id;
            packet_manager->send_packet(pubrel_packet);
            break;
        }

        case PacketType::Puback:
        case PacketType::Pubcomp: {

            DisconnectPacket disconnect_packet;
            packet_manager->send_packet(disconnect_packet);
            bufferevent_enable(packet_manager->bev, EV_WRITE);
            bufferevent_setcb(packet_manager->bev, packet_manager->bev->readcb, close_cb, NULL, NULL);

            break;
        }

        default:
            std::cerr << "unexpected packet type " << static_cast<int>(packet_ptr->type) << "\n";
            throw std::exception();

    }

}

void usage() {
    std::cout << "usage: client [options]\n";
}

void parse_arguments(int argc, char *argv[]) {
    static struct option longopts[] = {
            {"broker-host",   required_argument, NULL, 'b'},
            {"broker-port",   required_argument, NULL, 'p'},
            {"client-id",     required_argument, NULL, 'i'},
            {"topic",         required_argument, NULL, 't'},
            {"message",       required_argument, NULL, 'm'},
            {"qos",           required_argument, NULL, 'q'},
            {"clean-session", no_argument,       NULL, 'c'},
            {"help",          no_argument,       NULL, 'h'}
    };


    int ch;
    while ((ch = getopt_long(argc, argv, "b:p:i:t:m:q:c", longopts, NULL)) != -1) {
        switch (ch) {
            case 'b':
                broker_host = optarg;
                break;
            case 'p':
                broker_port = static_cast<uint16_t>(atoi(optarg));
                break;
            case 'i':
                client_id = optarg;
                break;
            case 't':
                topic = optarg;
                break;
            case 'm':
                message = std::vector<uint8_t>(optarg, optarg + strlen(optarg));
            case 'q':
                qos = static_cast<uint8_t>(atoi(optarg));
                break;
            case 'c':
                clean_session = true;
                break;
            case 'h':
                usage();
                std::exit(0);

            default:
                usage();
                std::exit(1);
        }
    }

}

static void close_cb(struct bufferevent *bev, void *arg) {
    std::cout << "close cb\n";
    if (evbuffer_get_length(bufferevent_get_output(bev)) == 0) {
        std::cout << "write buffer empty\n";
        bufferevent_free(bev);
        std::exit(0);
    }
}

uint16_t next_packet_id() {
    static uint16_t packet_id = 0;
    if (++packet_id == 0) {
        ++packet_id;
    }
    return packet_id;

}
