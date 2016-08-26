/**
 * @file client_sub.cc
 *
 * MQTT Subscriber (client)
 *
 * Connect to a listening broker and add a subscription a topic then listen for pubished messages from the broker.
 * Topic strings can follow the MQTT 3.1.1 specification including wildcards.
 * See [the MQTT specification](http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html)
 */

#include "packet.h"
#include "packet_manager.h"
#include "base_session.h"

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/dns.h>
#include <evdns.h>

#include <getopt.h>

#include <iostream>
#include <vector>
#include <csignal>

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
static void parse_arguments(int argc, char *argv[]);

/**
 * On broker connection callback.
 *
 * @param bev    Pointer to bufferevent internal control structure.
 * @param event  The bufferevent event code, on success this will be EV_EVENT_CONNECTED, it can also be one of
 * EV_EVENT_EOF, EV_EVENT_ERROR, or EV_EVENT_TIMEOUT.
 * @param arg    Pointer to user data passed in when callback is set.  Here this should be a pointer to the event_base
 * object.
 */
static void connect_event_cb(struct bufferevent *bev, short event, void *arg);

/**
 * Socket close handler.
 *
 * When the session is closing this function is set to be called when the write buffer is empty.  When called it will
 * close the connection and exit the event loop.
 *
 * @param bev Pointer to the bufferevent internal control structure.
 * @param arg Pointer to user data passed in when callback is set.  Here this should be a pointer to the event_base
 * object.
 */
static void close_cb(struct bufferevent *bev, void *arg);

/**
 * Callback run when SIGINT or SIGTERM is attached, will cleanly exit.
 *
 * @param signal Integer value of signal.
 * @param event  Should be EV_SIGNAL.
 * @param arg    Pointer originally passed to evsignal_new.
 */
static void signal_cb(evutil_socket_t, short event, void *);

/**
 * Options settable on through command line arguments.
 */
struct options_t {

    /** Broker host to connect to.  DNS name is allowed. */
    std::string broker_host = "localhost";

    /** Broker port. */
    uint16_t broker_port = 1883;

    /** Client id.  If empty no client id will be sent.  The broker will generate a client id automatically. */
    std::string client_id;

    /** Topics to subscribe to.  This option can be specified more than once to subscribe to multiple topics. */
    std::vector<std::string> topics;

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
 * Session class specialized for this publishing client.
 *
 * Override base class control packet handlers used to subscribe to topics and to receive messages forwarded from the
 * broker.  Any other control packets received will be handled by default methods, in most cases that will result in a
 * thrown exception.
 */
class ClientSession : public BaseSession {
public:

    /**
     * Constructor
     *
     * @param bev       Pointer to the buffer event structure for the broker connection.
     * @param options   Options structure.
     */
    ClientSession(bufferevent *bev, const options_t &options) : BaseSession(bev), options(options) {}

    /** Reference to the options structure with members updated from command line arguments. */
    const options_t &options;

    /**
     * Handle a connack control packet from the broker.
     *
     * This packet will be sent in response to the connect packet that this client will send.   Check the return code
     * and disconnect on any error.  Otherwise construct a subscribe packet from the options and send it to the broker.
     *
     * @param connack_packet  The connack control packet received.
     */
    void handle_connack(const ConnackPacket &connack_packet) override {

        SubscribePacket subscribe_packet;
        subscribe_packet.packet_id = packet_manager->next_packet_id();

        for (auto topic : options.topics) {
            subscribe_packet.subscriptions.push_back(Subscription{topic, options.qos});
        }
        packet_manager->send_packet(subscribe_packet);
    }

    /**
     * Handle a suback control packet from the broker.
     *
     * This packet will be sent in response to a subscribe message. Check the error code in the packet and if there is
     * an error output an error message but continue to listen for messages.  Also check the qos field in the suback
     * packet and compare with the requested qos in the original subscribe.  If there is a mismatch output a message.
     *
     * @param suback_packet The received suback control packet.
     */
    void handle_suback(const SubackPacket &suback_packet) override {

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

    /**
     * Handle a publish control packet from the broker.
     *
     * This packet will contain forwarded messages from other clients published to the topic. Output the message and
     * acknowledge based on the QoS in the packet.  In the case of a QoS 0 message there is no acknowledegment.
     * A Qos 1 message will require a Puback packet in response.  A QoS 2 message requires a Pubrec packet from the
     * subscriber followed by a Pubrel packet from the broker and finally a Pubcomp packet is sent from the subscriber.
     *
     * @param publish_packet the received publish packet
     */
    void handle_publish(const PublishPacket &publish_packet) override {

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


    /**
     * Handle a pubrel control packet from the broker.
     *
     * This packet will be sent in repsponse to a Pubrel packet in the QoS 2 protocol flow. Send a Pubcomp packet
     * in response.  This completes the QoS 2 flow.
     *
     * @param pubrel_packet The received pubrel control packet.
     */
    void handle_pubrel(const PubrelPacket &pubrel_packet) override {
        PubcompPacket pubcomp_packet;
        pubcomp_packet.packet_id = pubrel_packet.packet_id;
        packet_manager->send_packet(pubcomp_packet);
    }

    /**
     * Handle protocol events in the packet manager.
     *
     * This callback will be called by the packet manager in case of protocol or network error.  In most cases the
     * response is to close the connection to the broker.
     *
     * @param event The specific type of the event reported by the packet manager.
     */
    void packet_manager_event(PacketManager::EventType event) override {
        event_base_loopexit(packet_manager->bev->ev_base, NULL);
        BaseSession::packet_manager_event(event);
    }
};

/**
 * The session instance.
 *
 * MQTT requires that both the client and server maintain a session state.
 */
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
    std::cout <<
R"END(usage: mqtt_client_sub [options]

Connect to an MQTT broker and publish a single message to a single topic.

OPTIONS

--broker-host | -b        Broker host name or ip address, default localhost
--broker-port | -p        Broker port, default 1883
--client-id | -i          Client id, default none
--topic | -t              Topic string to subscribe to, this option can be provided more that once to subscribe
                          to multiple topics, default none
--qos | -q                QoS (Quality of Service), should be 0, 1, or 2, default 0
--clean-session | -c      Disable session persistence, default false
--help | -h               Display this message and exit
)END";

}

void parse_arguments(int argc, char *argv[]) {
    static struct option longopts[] = {
            {"broker-host",   required_argument, NULL, 'b'},
            {"broker-port",   required_argument, NULL, 'p'},
            {"client-id",     required_argument, NULL, 'i'},
            {"topic",         required_argument, NULL, 't'},
            {"qos",           required_argument, NULL, 'q'},
            {"clean-session", no_argument,       NULL, 'c'},
            {"help",          no_argument,       NULL, 'h'}
    };


    int ch;
    while ((ch = getopt_long(argc, argv, "b:p:i:t:q:ch", longopts, NULL)) != -1) {
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
