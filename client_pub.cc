//
// Created by Scott Brumbaugh on 8/11/16.
//

#include <iostream>
#include <string>
#include <cstdlib>
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

static void signal_cb(evutil_socket_t, short event, void *);

static void connect_event_cb(struct bufferevent *, short event, void *);

std::string broker_host = "localhost";
uint16_t broker_port = 1883;
std::string client_id;
std::vector<std::string> topics;
uint16_t qos = 0;
uint16_t keep_alive = 60;
bool clean_session = false;

ClientSession *session;

int main(int argc, char *argv[]) {

    struct event_base *evloop;
    struct event *signal_event;
    struct evdns_base *dns_base;
    struct bufferevent *bev;

    parse_arguments(argc, argv);

    std::cout << "broker host " << broker_host << "\n";
    std::cout << "broker port " << broker_port << "\n";
    std::cout << "client id " << client_id << "\n";
    for (auto topic : topics) {
        std::cout << "topic " << topic << "\n";
    }
    std::cout << "qos " << qos << "\n";
    std::cout << "keep alive " << keep_alive << "\n";
    std::cout << "clean session " << clean_session << "\n";

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

    event_base_dispatch(evloop);
    event_free(signal_event);
    event_base_free(evloop);

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
            {"qos",           required_argument, NULL, 'q'},
            {"keep-alive",    required_argument, NULL, 'k'},
            {"clean-session", no_argument,       NULL, 'c'},
            {"help",          no_argument,       NULL, 'h'}
    };


    int ch;
    while ((ch = getopt_long(argc, argv, "b:p:i:t:q:k:c", longopts, NULL)) != -1) {
        switch (ch) {
            case 'b':
                broker_host = optarg;
                break;
            case 'p':
                broker_port = atoi(optarg);
                break;
            case 'i':
                client_id = optarg;
                break;
            case 't':
                topics.push_back(optarg);
                break;
            case 'q':
                qos = atoi(optarg);
                break;
            case 'k':
                keep_alive = atoi(optarg);
                break;
            case 'c':
                clean_session = true;
                break;
            case 'h':
                usage();
                std::exit(0);
                break;
            default:
                usage();
                std::exit(1);
        }
    }

}

static void signal_cb(evutil_socket_t fd, short event, void *arg) {

    std::cout << "signal event\n";

    event_base *base = static_cast<event_base *>(arg);

    if (event_base_loopexit(base, NULL)) {
        std::cerr << "failed to exit event loop\n";
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

static void connect_event_cb(struct bufferevent *bev, short events, void *arg) {

    std::cout << "connect event\n";

    if (events & BEV_EVENT_CONNECTED) {
        std::cout << "Connect okay.\n";
        session = new ClientSession(bev);
        ConnectPacket connect_packet;
        connect_packet.client_id = "client1";
        session->send_connect(connect_packet);
        PublishPacket publish_packet;
        publish_packet.qos(1);
        publish_packet.topic_name = "first topic";
        std::string message("first message");
        publish_packet.packet_id = session->next_packet_id();
        publish_packet.message_data = std::vector<uint8_t>(message.begin(), message.end());
        session->send_publish(publish_packet);
        DisconnectPacket disconnect_packet;
        session->send_disconnect(disconnect_packet);

        bufferevent_enable(bev, EV_WRITE);
//        bufferevent_disable(bev, EV_READ);
        bufferevent_setcb(bev, bev->readcb, close_cb, NULL, NULL);

    } else if (events & (BEV_EVENT_ERROR | BEV_EVENT_EOF)) {
        struct event_base *base = static_cast<struct event_base *>(arg);
        if (events & BEV_EVENT_ERROR) {
            int err = bufferevent_socket_get_dns_error(bev);
            if (err)
                std::cerr << "DNS error: " << evutil_gai_strerror(err) << "\n";
        }
        printf("Closing\n");
        bufferevent_free(bev);
        event_base_loopexit(base, NULL);
    }

}
