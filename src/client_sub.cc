//
// Created by Scott Brumbaugh on 8/11/16.
//

#include <iostream>
#include <vector>
#include <csignal>

#include <getopt.h>

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/dns.h>
#include <evdns.h>

#include "mqtt.h"
#include "packet.h"
#include "packet_manager.h"
#include "base_session.h"

static void usage(void);

static void parse_arguments(int argc, char *argv[]);

static void connect_event_cb(struct bufferevent *, short event, void *);

static void close_cb(struct bufferevent *bev, void *arg);

static void signal_cb(evutil_socket_t, short event, void *);

struct options_t {
    std::string broker_host = "localhost";
    uint16_t broker_port = 1883;
    std::string client_id;
    std::vector<std::string> topics;
    QoSType qos = QoSType::QoS0;
    bool clean_session = false;
} options;

class ClientSession : public BaseSession
{
public:

    const options_t & options;

    ClientSession(bufferevent * bev, const options_t & options) : BaseSession(bev), options(options) {}

    void handle_connack(const ConnackPacket & connack_packet) override {
        SubscribePacket subscribe_packet;
        subscribe_packet.packet_id = packet_manager->next_packet_id();
        for (auto topic : options.topics) {
            subscribe_packet.subscriptions.push_back(Subscription{topic, options.qos});
        }
        packet_manager->send_packet(subscribe_packet);
    }

    void handle_suback(const SubackPacket & suback_packet) override {

        for (size_t i = 0; i < suback_packet.return_codes.size(); i++) {
            SubackPacket::ReturnCode code = suback_packet.return_codes[i];
            if (code == SubackPacket::ReturnCode::Failure) {
                std::cout << "Subscription to topic " << options.topics[i] << "failed\n";
            } else if (static_cast<QoSType>(code) != options.qos) {
                std::cout << "Topic " << options.topics[i] << " requested qos " << static_cast<int>(options.qos)
                          << " subscribed "
                          << static_cast<int>(code) << "\n";
            }
        }
    }

    void handle_publish(const PublishPacket & publish_packet) override {

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
    }

    void handle_pubrel(const PubrelPacket & pubrel_packet) override {
        PubcompPacket pubcomp_packet;
        pubcomp_packet.packet_id = pubrel_packet.packet_id;
        packet_manager->send_packet(pubcomp_packet);
    }

    void packet_manager_event(PacketManager::EventType event) override {
        event_base_loopexit(packet_manager->bev->ev_base, NULL);
        BaseSession::packet_manager_event(event);
     }
};

std::unique_ptr<ClientSession> session;

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

    bufferevent_socket_connect_hostname(bev, dns_base, AF_UNSPEC, options.broker_host.c_str(), options.broker_port);
    evdns_base_free(dns_base, 0);

    event_base_dispatch(evloop);

    event_free(signal_event);
    event_base_free(evloop);

}

static void connect_event_cb(struct bufferevent *bev, short events, void *arg) {

    if (events & BEV_EVENT_CONNECTED) {

        session = std::unique_ptr<ClientSession>(new ClientSession(bev, options));

        ConnectPacket connect_packet;
        connect_packet.client_id = options.client_id;
        connect_packet.clean_session(options.clean_session);
        session->packet_manager->send_packet(connect_packet);

    } else if (events & (BEV_EVENT_ERROR | BEV_EVENT_EOF)) {
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
                options.broker_host = optarg;
                break;
            case 'p':
                options.broker_port = static_cast<uint16_t>(atoi(optarg));
                break;
            case 'i':
                options.client_id = optarg;
                break;
            case 't':
                options.topics.push_back(optarg);
                break;
            case 'q':
                options.qos = static_cast<QoSType>(atoi(optarg));
                break;
            case 'c':
                options.clean_session = true;
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

    event_base *base = static_cast<event_base *>(arg);
    event_base_loopexit(base, NULL);
}

static void signal_cb(evutil_socket_t fd, short event, void *arg) {

    DisconnectPacket disconnect_packet;
    session->packet_manager->send_packet(disconnect_packet);

    bufferevent_disable(session->packet_manager->bev, EV_READ);
    bufferevent_setcb(session->packet_manager->bev, NULL, close_cb, NULL, arg);
}
