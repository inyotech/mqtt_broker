//
// Created by Scott Brumbaugh on 8/22/16.
//


#include "gtest/gtest.h"

#include "base_session.h"
#include "broker_session.h"
#include "session_manager.h"

#include <event2/listener.h>
#include <event2/buffer.h>
#include <event2/dns.h>

class SessionProtocol : public testing::Test {
public:

    struct event_base *evloop;
    struct evconnlistener *listener;

    SessionManager session_manager;

    void SetUp() {

        struct sockaddr_in sin;

        unsigned short listen_port = 1883;

        evloop = event_base_new();
        ASSERT_NE(evloop, nullptr);

        event *timer = evtimer_new(evloop, timeout_cb, NULL);
        timeval timeout = {5, 0};
        evtimer_add(timer, &timeout);

        std::memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_port = htons(listen_port);

        listener = evconnlistener_new_bind(evloop, listener_cb, this,
                                           LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1,
                                           (struct sockaddr *) &sin, sizeof(sin));

        ASSERT_TRUE(listener != NULL);


    }

    void TearDown() {

        evconnlistener_free(listener);
        event_base_free(evloop);

    }

    static void listener_cb(struct evconnlistener *listener, evutil_socket_t fd,
                            struct sockaddr *sa, int socklen, void *_this) {

        struct event_base *evbase = static_cast<SessionProtocol *>(_this)->evloop;
        struct bufferevent *bev;

        bev = bufferevent_socket_new(evbase, fd, BEV_OPT_CLOSE_ON_FREE);
        ASSERT_TRUE(bev != NULL);

        SessionManager &session_manager = static_cast<SessionProtocol *>(_this)->session_manager;

        session_manager.accept_connection(bev);
    }

    static void timeout_cb(int fd, short event, void *arg) {
        SessionProtocol *_this = static_cast<SessionProtocol *>(arg);

        ADD_FAILURE();

        event_base_loopexit(_this->evloop, NULL);
    }

};

template<typename SessionType>
class Client {

public:

    Client(event_base *evbase) : evloop(evbase) {}

    struct event_base *evloop;

    std::function<void()> on_ready = []() {};

    std::unique_ptr<SessionType> session;

    void connect_to_broker() {

        struct evdns_base *dns_base;

        bufferevent *bev = bufferevent_socket_new(evloop, -1, BEV_OPT_CLOSE_ON_FREE);

        bufferevent_setcb(bev, NULL, NULL, connect_event_cb, this);

        dns_base = evdns_base_new(evloop, 1);

        bufferevent_socket_connect_hostname(bev, dns_base, AF_UNSPEC, "localhost", 1883);

        evdns_base_free(dns_base, 0);

    }

    static void connect_event_cb(struct bufferevent *bev, short events, void *arg) {

        Client<SessionType> *_this = static_cast<Client<SessionType> *>(arg);

        _this->session = std::unique_ptr<SessionType>(new SessionType(bev));

        _this->session->on_ready = _this->on_ready;

        _this->session->connection_made();

    }

};

class TestSession : public BaseSession {

public:

    TestSession(bufferevent *bev) : BaseSession(bev) {}

    std::function<void()> on_ready;

    void connection_made() {
        ConnectPacket connect_packet;
        packet_manager->send_packet(connect_packet);
    }

    void disconnect_all() {

        DisconnectPacket disconnect_packet;
        packet_manager->send_packet(disconnect_packet);

        bufferevent_enable(packet_manager->bev, EV_WRITE);
        bufferevent_disable(packet_manager->bev, EV_READ);
        bufferevent_setcb(packet_manager->bev, NULL, close_cb, NULL,
                          bufferevent_get_base(packet_manager->bev));

    }

    static void close_cb(struct bufferevent *bev, void *arg) {
        if (evbuffer_get_length(bufferevent_get_output(bev)) == 0) {
            event_base *base = static_cast<event_base *>(arg);
            event_base_loopexit(base, NULL);;
        }
    }
};

struct TestParams {
    std::string test_topic;
    std::string test_message;
    QoSType qos;
};

static TestParams qos0_params = {
        .test_topic = "a/b/c",
        .test_message = "test message",
        .qos = QoSType::QoS0,
};

static TestParams qos1_params = {
        .test_topic = "a/b/c",
        .test_message = "test message",
        .qos = QoSType::QoS1,
};

static TestParams qos2_params = {
        .test_topic = "a/b/c",
        .test_message = "test message",
        .qos = QoSType::QoS2,
};

template<typename ::TestParams &params>
class SubscriberSession;

template<typename ::TestParams &params>
class PublisherSession;

TEST_F(SessionProtocol, qos0_test) {

    Client<PublisherSession<qos0_params>> publisher(evloop);

    Client<SubscriberSession<qos0_params>> subscriber(evloop);

    subscriber.on_ready = [&publisher]() { publisher.connect_to_broker(); };

    subscriber.connect_to_broker();

    event_base_dispatch(evloop);

}

TEST_F(SessionProtocol, qos1_test) {

    Client<PublisherSession<qos1_params>> publisher(evloop);

    Client<SubscriberSession<qos1_params>> subscriber(evloop);

    subscriber.on_ready = [&publisher]() { publisher.connect_to_broker(); };

    subscriber.connect_to_broker();

    event_base_dispatch(evloop);

}

TEST_F(SessionProtocol, qos2_test) {

    Client<PublisherSession<qos2_params>> publisher(evloop);

    Client<SubscriberSession<qos2_params>> subscriber(evloop);

    subscriber.on_ready = [&publisher]() { publisher.connect_to_broker(); };

    subscriber.connect_to_broker();

    event_base_dispatch(evloop);

}

template<typename ::TestParams &params>
class SubscriberSession : public TestSession {

public:

    SubscriberSession(bufferevent *bev) : TestSession(bev) {}

    void handle_connack(const ConnackPacket &connack_packet) override {

        SubscribePacket subscribe_packet;
        subscribe_packet.packet_id = packet_manager->next_packet_id();
        subscribe_packet.subscriptions.push_back(Subscription{params.test_topic, params.qos});
        packet_manager->send_packet(subscribe_packet);
    }

    void handle_suback(const SubackPacket &suback_packet) override {
        on_ready();
    }

    void handle_publish(const PublishPacket &publish_packet) override {

        ASSERT_EQ(publish_packet.message_data,
                  std::vector<uint8_t>(params.test_message.begin(), params.test_message.end()));

        disconnect_all();
    }
};

template<typename ::TestParams &params>
class PublisherSession : public TestSession {
public:

    PublisherSession(bufferevent *bev) : TestSession(bev) {}

    void handle_connack(const ConnackPacket &connack_packet) override {

        PublishPacket publish_packet;
        publish_packet.qos(params.qos);
        publish_packet.topic_name = params.test_topic;
        publish_packet.packet_id = packet_manager->next_packet_id();
        publish_packet.message_data = std::vector<uint8_t>(params.test_message.begin(),
                                                           params.test_message.end());
        packet_manager->send_packet(publish_packet);

        if (params.qos == QoSType::QoS0) {
            DisconnectPacket disconnect_packet;
            packet_manager->send_packet(disconnect_packet);
        }
    }

    void handle_puback(const PubackPacket &puback_packet) override {
        ASSERT_EQ(params.qos, QoSType::QoS1);

        DisconnectPacket disconnect_packet;
        packet_manager->send_packet(disconnect_packet);
    }

    void handle_pubrec(const PubrecPacket &pubrec_packet) override {
        ASSERT_EQ(params.qos, QoSType::QoS2);

        PubrelPacket pubrel_packet;
        pubrel_packet.packet_id = pubrec_packet.packet_id;
        packet_manager->send_packet(pubrel_packet);
    }

    void handle_pubcomp(const PubcompPacket &pubcomp_packet) override {
        ASSERT_EQ(params.qos, QoSType::QoS2);

        DisconnectPacket disconnect_packet;
        packet_manager->send_packet(disconnect_packet);
    }

};
