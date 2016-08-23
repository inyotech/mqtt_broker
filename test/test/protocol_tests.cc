//
// Created by Scott Brumbaugh on 8/22/16.
//


#include "gtest/gtest.h"

#include "session.h"
#include "session_manager.h"

#include <event2/listener.h>
#include <event2/dns.h>

class Protocol : public testing::Test {
public:

    struct event_base *evloop;
    struct evconnlistener *listener;

    std::unique_ptr<PacketManager> packet_manager;
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

        struct event_base *evbase = static_cast<Protocol *>(_this)->evloop;
        struct bufferevent *bev;

        bev = bufferevent_socket_new(evbase, fd, BEV_OPT_CLOSE_ON_FREE);
        ASSERT_TRUE(bev != NULL);

        SessionManager &session_manager = static_cast<Protocol *>(_this)->session_manager;

        session_manager.accept_connection(bev);
    }

    static void connect_event_cb(struct bufferevent *bev, short events, void *arg) {

        Protocol *_this = static_cast<Protocol *>(arg);

        _this->packet_manager = std::unique_ptr<PacketManager>(new PacketManager(bev));
        _this->packet_manager->set_packet_received_handler(
                std::bind(&Protocol::packet_received_callback, _this, std::placeholders::_1));

        _this->connection_made();
    }

    static void timeout_cb(int fd, short event, void *arg) {
        Protocol *_this = static_cast<Protocol *>(arg);

        ADD_FAILURE();

        event_base_loopexit(_this->evloop, NULL);
    }

    void connect_to_broker() {

        struct evdns_base *dns_base;

        bufferevent *bev = bufferevent_socket_new(evloop, -1, BEV_OPT_CLOSE_ON_FREE);

        bufferevent_setcb(bev, NULL, NULL, connect_event_cb, this);

        dns_base = evdns_base_new(evloop, 1);

        bufferevent_socket_connect_hostname(bev, dns_base, AF_UNSPEC, "localhost", 1883);

        evdns_base_free(dns_base, 0);

    }

    virtual void connection_made() = 0;

    virtual void packet_received_callback(std::unique_ptr<Packet> packet) = 0;

};

class Connect : public Protocol {

    virtual void connection_made() {

        ConnectPacket connect_packet;
        packet_manager->send_packet(connect_packet);

    }

    virtual void packet_received_callback(std::unique_ptr<Packet> packet) {

        ASSERT_EQ(packet->type, PacketType::Connack);
        ConnackPacket &connack_packet = dynamic_cast<ConnackPacket &>(*packet);
        ASSERT_EQ(connack_packet.return_code, ConnackPacket::ReturnCode::Accepted);

        DisconnectPacket disconnect_packet;
        packet_manager->send_packet(disconnect_packet);

        event_base_loopexit(evloop, NULL);

    }

};

class Ping : public Protocol {

    virtual void connection_made() {
        PingreqPacket pingreq_packet;
        packet_manager->send_packet(pingreq_packet);
    }

    virtual void packet_received_callback(std::unique_ptr<Packet> packet) {

        ASSERT_EQ(packet->type, PacketType::Pingresp);

        DisconnectPacket disconnect_packet;
        packet_manager->send_packet(disconnect_packet);

        event_base_loopexit(evloop, NULL);
    }

};

class Subscribe : public Protocol {

    uint16_t subscribe_packet_id;
    std::vector<std::pair<std::string, QoSType>> subscriptions = {
            {"a/b/c", QoSType::QoS0},
            {"d/e/f", QoSType::QoS1},
            {"g/h/i", QoSType::QoS2},
    };

    virtual void connection_made() {

        ConnectPacket connect_packet;
        packet_manager->send_packet(connect_packet);

        SubscribePacket subscribe_packet;
        subscribe_packet.packet_id = this->packet_manager->next_packet_id();
        subscribe_packet_id = subscribe_packet.packet_id;

        for (auto subscription : subscriptions) {
            subscribe_packet.subscriptions.push_back(Subscription{subscription.first, subscription.second});
        }
        packet_manager->send_packet(subscribe_packet);

    }

    virtual void packet_received_callback(std::unique_ptr<Packet> packet) {

        ASSERT_EQ(packet->type, PacketType::Connack);
        ConnackPacket &connack_packet = dynamic_cast<ConnackPacket &>(*packet);
        ASSERT_EQ(connack_packet.return_code, ConnackPacket::ReturnCode::Accepted);

        this->packet_manager->set_packet_received_handler(
                std::bind(&Subscribe::suback_received_callback, this, std::placeholders::_1));

    }

    void suback_received_callback(std::unique_ptr<Packet> packet) {

        ASSERT_EQ(packet->type, PacketType::Suback);
        SubackPacket &suback_packet = dynamic_cast<SubackPacket &>(*packet);
        ASSERT_EQ(suback_packet.packet_id, subscribe_packet_id);

        ASSERT_EQ(suback_packet.return_codes.size(), 3);

        ASSERT_EQ(suback_packet.return_codes[0], SubackPacket::ReturnCode::SuccessQoS0);
        ASSERT_EQ(suback_packet.return_codes[1], SubackPacket::ReturnCode::SuccessQoS1);
        ASSERT_EQ(suback_packet.return_codes[2], SubackPacket::ReturnCode::SuccessQoS2);

        DisconnectPacket disconnect_packet;
        packet_manager->send_packet(disconnect_packet);

        event_base_loopexit(evloop, NULL);
    }

};

class Unsubscribe : public Protocol {

    uint16_t unsubscribe_packet_id;
    std::vector<std::string> topics = {
            "a/b/c",
            "d/e/f",
            "g/h/i",
    };

    virtual void connection_made() {

        ConnectPacket connect_packet;
        packet_manager->send_packet(connect_packet);

        UnsubscribePacket unsubscribe_packet;
        unsubscribe_packet.packet_id = this->packet_manager->next_packet_id();
        unsubscribe_packet_id = unsubscribe_packet.packet_id;

        for (auto topic : topics) {
            unsubscribe_packet.topics.push_back(topic);
        }

        packet_manager->send_packet(unsubscribe_packet);
    }

    virtual void packet_received_callback(std::unique_ptr<Packet> packet) {

        ASSERT_EQ(packet->type, PacketType::Connack);
        ConnackPacket &connack_packet = dynamic_cast<ConnackPacket &>(*packet);
        ASSERT_EQ(connack_packet.return_code, ConnackPacket::ReturnCode::Accepted);

        this->packet_manager->set_packet_received_handler(
                std::bind(&Unsubscribe::unsuback_received_callback, this, std::placeholders::_1));

    }

    void unsuback_received_callback(std::unique_ptr<Packet> packet) {

        ASSERT_EQ(packet->type, PacketType::Unsuback);
        UnsubackPacket &unsuback_packet = dynamic_cast<UnsubackPacket &>(*packet);
        ASSERT_EQ(unsuback_packet.packet_id, unsubscribe_packet_id);

        DisconnectPacket disconnect_packet;
        packet_manager->send_packet(disconnect_packet);

        event_base_loopexit(evloop, NULL);
    }

};

class PublishQoS0 : public Protocol {

    std::string topic = "a/b/c";
    std::string message_data = "a test message";
    QoSType qos = QoSType::QoS0;

    virtual void connection_made() {

        ConnectPacket connect_packet;
        packet_manager->send_packet(connect_packet);

        PublishPacket publish_packet;
        publish_packet.packet_id = this->packet_manager->next_packet_id();

        publish_packet.topic_name = topic;
        publish_packet.message_data = std::vector<uint8_t>(message_data.begin(), message_data.end());
        publish_packet.qos(qos);
        packet_manager->send_packet(publish_packet);

        DisconnectPacket disconnect_packet;
        packet_manager->send_packet(disconnect_packet);

        event_base_loopexit(evloop, NULL);
    }

    virtual void packet_received_callback(std::unique_ptr<Packet> packet) {

        ADD_FAILURE();

    }
};

class PublishQoS1 : public Protocol {

    uint16_t publish_packet_id;
    std::string topic = "a/b/c";
    std::string message_data = "a test message";
    QoSType qos = QoSType::QoS1;

    virtual void connection_made() {

        ConnectPacket connect_packet;
        packet_manager->send_packet(connect_packet);

        PublishPacket publish_packet;
        publish_packet.packet_id = this->packet_manager->next_packet_id();
        publish_packet_id = publish_packet.packet_id;

        publish_packet.topic_name = topic;
        publish_packet.message_data = std::vector<uint8_t>(message_data.begin(), message_data.end());
        publish_packet.qos(qos);
        packet_manager->send_packet(publish_packet);

        DisconnectPacket disconnect_packet;
        packet_manager->send_packet(disconnect_packet);
    }

    virtual void packet_received_callback(std::unique_ptr<Packet> packet) {

        ASSERT_EQ(packet->type, PacketType::Connack);
        ConnackPacket &connack_packet = dynamic_cast<ConnackPacket &>(*packet);
        ASSERT_EQ(connack_packet.return_code, ConnackPacket::ReturnCode::Accepted);

        this->packet_manager->set_packet_received_handler(
                std::bind(&PublishQoS1::puback_received_callback, this, std::placeholders::_1));

    }

    void puback_received_callback(std::unique_ptr<Packet> packet) {

        ASSERT_EQ(packet->type, PacketType::Puback);
        PubackPacket &puback_packet = dynamic_cast<PubackPacket &>(*packet);
        ASSERT_EQ(puback_packet.packet_id, publish_packet_id);

        DisconnectPacket disconnect_packet;
        packet_manager->send_packet(disconnect_packet);

        event_base_loopexit(evloop, NULL);
    }

};

class PublishQoS2 : public Protocol {

    uint16_t publish_packet_id;
    std::string topic = "a/b/c";
    std::string message_data = "a test message";
    QoSType qos = QoSType::QoS2;

    virtual void connection_made() {

        ConnectPacket connect_packet;
        packet_manager->send_packet(connect_packet);

        PublishPacket publish_packet;
        publish_packet.packet_id = this->packet_manager->next_packet_id();
        publish_packet_id = publish_packet.packet_id;

        publish_packet.topic_name = topic;
        publish_packet.message_data = std::vector<uint8_t>(message_data.begin(), message_data.end());
        publish_packet.qos(qos);
        packet_manager->send_packet(publish_packet);

    }

    virtual void packet_received_callback(std::unique_ptr<Packet> packet) {

        ASSERT_EQ(packet->type, PacketType::Connack);
        ConnackPacket &connack_packet = dynamic_cast<ConnackPacket &>(*packet);
        ASSERT_EQ(connack_packet.return_code, ConnackPacket::ReturnCode::Accepted);

        this->packet_manager->set_packet_received_handler(
                std::bind(&PublishQoS2::pubrec_received_callback, this, std::placeholders::_1));

    }

    void pubrec_received_callback(std::unique_ptr<Packet> packet) {

        ASSERT_EQ(packet->type, PacketType::Pubrec);
        PubrecPacket &pubrec_packet = dynamic_cast<PubrecPacket &>(*packet);
        ASSERT_EQ(pubrec_packet.packet_id, publish_packet_id);

        PubrelPacket pubrel_packet;
        pubrel_packet.packet_id = pubrec_packet.packet_id;
        packet_manager->send_packet(pubrel_packet);

        this->packet_manager->set_packet_received_handler(
                std::bind(&PublishQoS2::pubcomp_received_callback, this, std::placeholders::_1));

    }

    void pubcomp_received_callback(std::unique_ptr<Packet> packet) {

        ASSERT_EQ(packet->type, PacketType::Pubcomp);
        PubcompPacket &pubcomp_packet = dynamic_cast<PubcompPacket &>(*packet);
        ASSERT_EQ(pubcomp_packet.packet_id, publish_packet_id);

        DisconnectPacket disconnect_packet;
        packet_manager->send_packet(disconnect_packet);

        event_base_loopexit(evloop, NULL);
    }
};

TEST_F(Connect, connection) {

    connect_to_broker();

    event_base_dispatch(evloop);
}

TEST_F(Ping, ping) {

    connect_to_broker();

    event_base_dispatch(evloop);
}

TEST_F(Subscribe, subscribe) {

    connect_to_broker();

    event_base_dispatch(evloop);
}

TEST_F(Unsubscribe, unsubscribe) {

    connect_to_broker();

    event_base_dispatch(evloop);
}

TEST_F(PublishQoS0, publish_qos0) {

    connect_to_broker();

    event_base_dispatch(evloop);
}

TEST_F(PublishQoS1, publish_qos1) {

    connect_to_broker();

    event_base_dispatch(evloop);
}

TEST_F(PublishQoS2, publish_qos2) {

    connect_to_broker();

    event_base_dispatch(evloop);
}