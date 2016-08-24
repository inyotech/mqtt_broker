//
// Created by Scott Brumbaugh on 8/11/16.
//

#include <iostream>
#include <vector>
#include <cstring>

#include <getopt.h>

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/dns.h>
#include <evdns.h>

#include "mqtt.h"
#include "packet.h"
#include "session_base.h"

static void usage(void);

static void parse_arguments(int argc, char *argv[]);

static void connect_event_cb(struct bufferevent *, short event, void *);

static void close_cb(struct bufferevent *bev, void *arg);

struct options_t {
    std::string broker_host = "localhost";
    uint16_t broker_port = 1883;
    std::string client_id;
    std::string topic;
    std::vector<uint8_t> message;
    QoSType qos = QoSType::QoS0;
    bool clean_session = false;
} options;

class ClientSession : public SessionBase {
public:
    const options_t &options;

    ClientSession(bufferevent *bev, const options_t &options) : SessionBase(bev), options(options) {}

    void handle_connack(const ConnackPacket &connack_packet) override {

        PublishPacket publish_packet;
        publish_packet.qos(options.qos);
        publish_packet.topic_name = options.topic;
        publish_packet.packet_id = packet_manager->next_packet_id();
        publish_packet.message_data = std::vector<uint8_t>(options.message.begin(), options.message.end());
        packet_manager->send_packet(publish_packet);

        if (options.qos == QoSType::QoS0) {
            disconnect();
        }
    }

    void handle_puback(const PubackPacket &puback_packet) override {
        disconnect();
    }

    void handle_pubrec(const PubrecPacket &pubrec_packet) override {
        PubrelPacket pubrel_packet;
        pubrel_packet.packet_id = pubrec_packet.packet_id;
        packet_manager->send_packet(pubrel_packet);

    }

    void handle_pubcomp(const PubcompPacket &puback_packet) override {
        disconnect();
    }

    void disconnect() {
        DisconnectPacket disconnect_packet;
        packet_manager->send_packet(disconnect_packet);
        bufferevent_enable(packet_manager->bev, EV_WRITE);
        bufferevent_setcb(packet_manager->bev, packet_manager->bev->readcb, close_cb, NULL,
                          packet_manager->bev->ev_base);
    }
};

std::unique_ptr<ClientSession> session;

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

    bufferevent_socket_connect_hostname(bev, dns_base, AF_UNSPEC, options.broker_host.c_str(), options.broker_port);

    evdns_base_free(dns_base, 0);

    event_base_dispatch(evloop);
    event_base_free(evloop);

}

static void connect_event_cb(struct bufferevent *bev, short events, void *arg) {

    if (events & BEV_EVENT_CONNECTED) {

        session = std::unique_ptr<ClientSession>(new ClientSession(bev, options));

        ConnectPacket connect_packet;
        connect_packet.client_id = options.client_id;
        session->packet_manager->send_packet(connect_packet);

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
                options.topic = optarg;
                break;
            case 'm':
                options.message = std::vector<uint8_t>(optarg, optarg + strlen(optarg));
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
    if (evbuffer_get_length(bufferevent_get_output(bev)) == 0) {
        event_base *base = static_cast<event_base *>(arg);
        event_base_loopexit(base, NULL);;
    }
}
