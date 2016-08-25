#pragma once

#include "session_base.h"
#include "packet_manager.h"
#include "packet.h"

#include <list>
#include <memory>

#include <event2/bufferevent.h>

class SessionManager;

class Message;

class Subscription;

class Session : public SessionBase {

public:

    Session(struct bufferevent *bev, SessionManager &session_manager) : SessionBase(bev),
                                                                        session_manager(session_manager) {
    }

    bool authorize_connection(const ConnectPacket &);

    void resume_session(std::unique_ptr<Session> &session,
                        std::unique_ptr<PacketManager> packet_manager);

    void send_pending_message(void);

    void forward_packet(const PublishPacket &packet);

    bool clean_session;

    std::vector<Subscription> subscriptions;

    // qos1 messages waiting for puback, will be moved between
    // sessions as part of session state
    std::vector<PublishPacket> qos1_pending_puback;

    // qos2 messages waiting for pubrec, will be moved between
    // sessions as part of session state
    std::vector<PublishPacket> qos2_pending_pubrec;

    // qos2 messages waiting for pubrel, will be moved between
    // sessions as part of session state
    std::vector<uint16_t> qos2_pending_pubrel;

    // qos2 pubrel packet ids waiting for pubcomp, will be moved
    // between sessions as part of session state
    std::vector<uint16_t> qos2_pending_pubcomp;

    void packet_received(std::unique_ptr<Packet>) override;

    void packet_manager_event(PacketManager::EventType event) override;

    void handle_connect(const ConnectPacket &) override;

    void handle_publish(const PublishPacket &) override;

    void handle_puback(const PubackPacket &) override;

    void handle_pubrec(const PubrecPacket &) override;

    void handle_pubrel(const PubrelPacket &) override;

    void handle_pubcomp(const PubcompPacket &) override;

    void handle_subscribe(const SubscribePacket &) override;

    void handle_unsubscribe(const UnsubscribePacket &) override;

    void handle_pingreq(const PingreqPacket &) override;

    void handle_disconnect(const DisconnectPacket &) override;

    SessionManager &session_manager;

};

