/**
 * @file client_pub.cc
 *
 * MQTT Publisher (client)
 *
 * Connect to a listening broker and publish a message on a topic.  Topic and message are passed as command line
 * arguments to this program.
 * See [the MQTT specification](http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html)
 */
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
#include "session_base.h"

/**
 * Display usage message
 *
 * Displays help on command line arguments.
 */
static void usage(void);

/**
 * Parse command line.
 *
 * Recognized command line arguments are parsed and added to the options instance.  This options instance will be
 * passed to the session instance.
 *
 * @param argc Command line argument count.
 * @param argv Command line argument values.
 */
static void parse_arguments(int argc, char * argv[]);


static void connect_event_cb(struct bufferevent * bev, short event, void * arg);

static void close_cb(struct bufferevent * bev, void * arg);

/**
 * Options settable on through command line arguments.
 */
static struct options_t {

    /** Broker host to connect to.  DNS name is allowed. */
    std::string broker_host = "localhost";

    /** Broker port. */
    uint16_t broker_port = 1883;

    /** Client id.  If empty no client id will be sent.  The broker will generate a client id automatically. */
    std::string client_id;

    /** Topic to publish to. */
    std::string topic;

    /** Message text to publish. */
    std::vector<uint8_t> message;

    /** Quality of service specifier for message. */
    QoSType qos = QoSType::QoS0;

    /**
     * Drop this session on exit if true.  If false this session will persist in the broker after disconnection
     * and will be continued at the next connection with the same client id.  This feature isn't useful when a client
     * id is not provided.
     */
    bool clean_session = false;

} options;

/**
 * Session class specialized for this client.
 *
 * Override base class control packet handlers used in message publishing.  Any control packets received be handled by
 * the default methods.  In most cases that will result in a thrown exception.
 */
class ClientSession : public SessionBase {

public:

    /**
     * Constructor
     *
     * @param bev       Pointer to the buffer event structure for the broker connection.
     * @param options   Options structure.
     */
    ClientSession(bufferevent *bev, const options_t &options) : SessionBase(bev), options(options) {}

    /** Reference to the options structure with members updated from command line arguments. */
    const options_t &options;

    /**
     * Store the packet id of the publish packet we send for comparisison with any puback or pubcomp control packets
     * received from the broker.
     */
    uint16_t published_packet_id = 0;

    /**
     * Handle a connack control packet from the broker.
     *
     * This packet will be sent in response to the connect packet that this client will send.   Check the return code
     * and disconnect on any error.  Otherwise construct a publish packet from the options and send this to the broker.
     *
     * @param connack_packet  The connack control packet received.
     */
    void handle_connack(const ConnackPacket &connack_packet) override {

        if (connack_packet.return_code != ConnackPacket::ReturnCode::Accepted) {
            std::cerr << "connection not accepted by broker\n";
            disconnect();
        }

        PublishPacket publish_packet;
        publish_packet.qos(options.qos);
        publish_packet.topic_name = options.topic;
        publish_packet.packet_id = packet_manager->next_packet_id();
        published_packet_id = publish_packet.packet_id;
        publish_packet.message_data = std::vector<uint8_t>(options.message.begin(), options.message.end());
        packet_manager->send_packet(publish_packet);

        if (options.qos == QoSType::QoS0) {
            disconnect();
        }
    }

    /**
     * Handle a puback control packet from the broker.
     *
     * This packet will be sent in response to a publish message with QoS 1.  Compare the packet id with the packet
     * id sent in a previous publish message.  Notify in case of a mismatch.
     *
     * @param puback_packet The received puback control packet.
     */
    void handle_puback(const PubackPacket &puback_packet) override {

        if (puback_packet.packet_id != published_packet_id) {
            std::cout << "puback packet id mismatch: sent " << published_packet_id << " received "
                      << puback_packet.packet_id << "\n";
        }
        disconnect();
    }

    /**
     * Handle a pubrec control packet from the broker.
     *
     * This packet will be sent in repsponse to a publish message with QoS 2.  Compare the packet id with the packet
     * id sent in a previous publish message.  Notify in case of mismatch.  Send a pubrel packet containing the received
     * packet id in response. Wait for pubcomp control packet response.
     *
     * @param pubrec_packet The received pubrec control packet.
     */
    void handle_pubrec(const PubrecPacket &pubrec_packet) override {

        if (pubrec_packet.packet_id != published_packet_id) {
            std::cout << "pubrec packet id mismatch: sent " << published_packet_id << " received "
                      << pubrec_packet.packet_id << "\n";
        }

        PubrelPacket pubrel_packet;
        pubrel_packet.packet_id = pubrec_packet.packet_id;
        packet_manager->send_packet(pubrel_packet);

    }

    /**
     * Handle a pubcomp control packet from the broker.
     *
     * This packet will be sent in response to a pubrel packet confirmation of a QoS 2 publish message.  Compare the
     * packet id sent in a previous publish message.  Notify in case of mismatch.  On receipt of this message the QoS 2
     * confirmation exchange is complete.  If the packet id matches the the packet id of the original message
     * disconnect from the broker, otherwise stay connected waiting for the pubcomp with the required packet id.
     *
     * @param puback_packet the received pubcomp packet
     */
    void handle_pubcomp(const PubcompPacket &pubcomp_packet) override {

        if (pubcomp_packet.packet_id != published_packet_id) {
            std::cout << "pubcomp packet id mismatch: sent " << published_packet_id << " received "
                      << pubcomp_packet.packet_id << "\n";

        } else {
            disconnect();
        }
    }

    /**
     * Disconnect from the broker.
     *
     * Send a disconnect control packet.  Set up libevent to wait for the disconnect packet to be written then close
     * the network connection.
     */
    void disconnect() {
        DisconnectPacket disconnect_packet;
        packet_manager->send_packet(disconnect_packet);
        bufferevent_enable(packet_manager->bev, EV_WRITE);
        bufferevent_setcb(packet_manager->bev, packet_manager->bev->readcb, close_cb, NULL,
                          packet_manager->bev->ev_base);
    }
};

/** The session instance. */

static std::unique_ptr<ClientSession> session;

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

static void usage() {
    std::cout << "usage: client [options]\n";
}

static void parse_arguments(int argc, char *argv[]) {
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
