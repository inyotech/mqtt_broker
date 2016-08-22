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
#include "packet.h"
#include "packet_manager.h"

static void usage(void);

static void parse_arguments(int argc, char *argv[]);

static void connect_event_cb(struct bufferevent *, short event, void *);

static void close_cb(struct bufferevent *bev, void *arg);

static void signal_cb(evutil_socket_t, short event, void *);

static void packet_received_callback(owned_packet_ptr_t Packet);

std::string broker_host = "localhost";
uint16_t broker_port = 1883;
std::string client_id;
std::vector<std::string> topics;
QoSType qos = QoSType::QoS0;
bool clean_session = false;

PacketManager *packet_manager;

int main(int argc, char *argv[]) {

    struct event_base *evloop;
    struct event *signal_event;
    struct evdns_base *dns_base;
    struct bufferevent *bev;

    parse_arguments(argc, argv);

    evloop = event_base_new();
    if (!evloop) {
        std::cerr << "Could not initialize libevent\n";
        std::exit(1);
    }

    signal_event = evsignal_new(evloop, SIGINT, signal_cb, evloop);
    evsignal_add(signal_event, NULL);
    signal_event = evsignal_new(evloop, SIGTERM, signal_cb, evloop);
    evsignal_add(signal_event, NULL);

    dns_base = evdns_base_new(evloop, 1);

    bev = bufferevent_socket_new(evloop, -1, BEV_OPT_CLOSE_ON_FREE);

    bufferevent_setcb(bev, NULL, NULL, connect_event_cb, evloop);

    bufferevent_socket_connect_hostname(bev, dns_base, AF_UNSPEC, broker_host.c_str(), broker_port);
    evdns_base_free(dns_base, 0);

    event_base_dispatch(evloop);
    event_free(signal_event);
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

            SubscribePacket subscribe_packet;
            subscribe_packet.packet_id = packet_manager->next_packet_id();
            for (auto topic : topics) {
                subscribe_packet.subscriptions.push_back(Subscription{topic, qos});
            }
            packet_manager->send_packet(subscribe_packet);
            break;
        }

        case PacketType::Suback: {

            SubackPacket &suback_packet = dynamic_cast<SubackPacket &>(*packet_ptr);

            for (int i = 0; i < suback_packet.return_codes.size(); i++) {
                SubackPacket::ReturnCode code = suback_packet.return_codes[i];
                if (code == SubackPacket::ReturnCode::Failure) {
                    std::cout << "Subscription to topic " << topics[i] << "failed\n";
                } else if (static_cast<QoSType>(code) != qos) {
                    std::cout << "Topic " << topics[i] << " requested qos " << static_cast<int>(qos) << " subscribed "
                              << static_cast<int>(code) << "\n";
                }
            }
            break;
        }

        case PacketType::Publish: {
            PublishPacket &publish_packet = dynamic_cast<PublishPacket &>(*packet_ptr);
            std::cout << std::string(publish_packet.message_data.begin(), publish_packet.message_data.end()) << "\n";
            if (publish_packet.qos() == QoSType::QoS1) {
                PubackPacket puback_packet;
                puback_packet.packet_id = publish_packet.packet_id;
                packet_manager->send_packet(puback_packet);
            } else if (publish_packet.qos() == QoSType::QoS2) {
                PubrecPacket pubrec_packet;
                pubrec_packet.packet_id = publish_packet.packet_id;
                packet_manager->send_packet(pubrec_packet);
            }
            break;
        }

        case PacketType::Pubrel: {

            PubcompPacket pubcomp_packet;
            pubcomp_packet.packet_id = dynamic_cast<PubrelPacket &>(*packet_ptr).packet_id;
            packet_manager->send_packet(pubcomp_packet);
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
                topics.push_back(optarg);
                break;
            case 'q':
                qos = static_cast<QoSType>(atoi(optarg));
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

    bufferevent_free(bev);
    event_base *base = static_cast<event_base *>(arg);
    event_base_loopexit(base, NULL);
}

static void signal_cb(evutil_socket_t fd, short event, void *arg) {

    std::cout << "signal_event\n";
    DisconnectPacket disconnect_packet;
    packet_manager->send_packet(disconnect_packet);

    bufferevent_disable(packet_manager->bev, EV_READ);
    bufferevent_setcb(packet_manager->bev, NULL, close_cb, NULL, arg);
}
