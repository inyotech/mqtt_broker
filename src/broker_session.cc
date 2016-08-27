/**
 * @file broker_session.cc
 */

#include "broker_session.h"
#include "session_manager.h"

#include <algorithm>

bool BrokerSession::authorize_connection(const ConnectPacket &packet) {
    return true;
}

void BrokerSession::resume_session(std::unique_ptr<BrokerSession> &session,
                                   std::unique_ptr<PacketManager> packet_manager_ptr) {

    packet_manager_ptr->set_event_handler(
            std::bind(&BrokerSession::packet_manager_event, session.get(), std::placeholders::_1));
    packet_manager_ptr->set_packet_received_handler(
            std::bind(&BrokerSession::packet_received, session.get(), std::placeholders::_1));
    session->packet_manager = std::move(packet_manager_ptr);

    ConnackPacket connack;

    connack.session_present(true);
    connack.return_code = ConnackPacket::ReturnCode::Accepted;

    session->packet_manager->send_packet(connack);
}

void BrokerSession::forward_packet(const PublishPacket &packet) {

    if (packet.qos() == QoSType::QoS0) {
        packet_manager->send_packet(packet);
    } else if (packet.qos() == QoSType::QoS1) {
        PublishPacket packet_to_send(packet);
        packet_to_send.dup(false);
        packet_to_send.retain(false);
        packet_to_send.packet_id = packet_manager->next_packet_id();
        qos1_pending_puback.push_back(packet_to_send);
        packet_manager->send_packet(packet_to_send);
    } else if (packet.qos() == QoSType::QoS2) {

        PublishPacket packet_to_send(packet);
        packet_to_send.dup(false);
        packet_to_send.retain(false);
        packet_to_send.packet_id = packet_manager->next_packet_id();

        auto previous_packet = find_if(qos2_pending_pubrec.begin(), qos2_pending_pubrec.end(),
                                       [&packet](const PublishPacket &p) { return p.packet_id == packet.packet_id; });
        if (previous_packet == qos2_pending_pubrec.end()) {
            qos2_pending_pubrec.push_back(packet_to_send);
        }

        packet_manager->send_packet(packet_to_send);

    }
}

void BrokerSession::send_pending_message() {

    if (!qos1_pending_puback.empty()) {
        packet_manager->send_packet(qos1_pending_puback[0]);
    } else if (!qos2_pending_pubrec.empty()) {
        packet_manager->send_packet(qos2_pending_pubrec[0]);
    } else if (!qos2_pending_pubrel.empty()) {
        PubrecPacket pubrec_packet;
        pubrec_packet.packet_id = qos2_pending_pubrel[0];
        packet_manager->send_packet(pubrec_packet);
    } else if (!qos2_pending_pubcomp.empty()) {
        PubrelPacket pubrel_packet;
        pubrel_packet.packet_id = qos2_pending_pubcomp[0];
        packet_manager->send_packet(pubrel_packet);
    }

    return;
}

void BrokerSession::packet_received(std::unique_ptr<Packet> packet) {
    BaseSession::packet_received(std::move(packet));
    send_pending_message();
}

void BrokerSession::packet_manager_event(PacketManager::EventType event) {
    BaseSession::packet_manager_event(event);
    if (clean_session) {
        session_manager.erase_session(this);
    }
}

void BrokerSession::handle_connect(const ConnectPacket &packet) {

    if (!authorize_connection(packet)) {
        session_manager.erase_session(this);
        return;
    }

    if (packet.clean_session()) {
        session_manager.erase_session(packet.client_id);
    } else {
        auto previous_session_it = session_manager.find_session(packet.client_id);
        if (previous_session_it != session_manager.sessions.end()) {
            std::unique_ptr<BrokerSession> &previous_session_ptr = *previous_session_it;
            resume_session(previous_session_ptr, std::move(packet_manager));
            session_manager.erase_session(this);
            return;
        }
    }

    client_id = packet.client_id;
    clean_session = packet.clean_session();

    ConnackPacket connack;

    connack.session_present(false);
    connack.return_code = ConnackPacket::ReturnCode::Accepted;

    packet_manager->send_packet(connack);

}

void BrokerSession::handle_publish(const PublishPacket &packet) {

    if (packet.qos() == QoSType::QoS0) {

        session_manager.handle_publish(packet);

    } else if (packet.qos() == QoSType::QoS1) {

        session_manager.handle_publish(packet);
        PubackPacket puback;
        puback.packet_id = packet.packet_id;
        packet_manager->send_packet(puback);

    } else if (packet.qos() == QoSType::QoS2) {

        auto previous_packet = find_if(qos2_pending_pubrel.begin(), qos2_pending_pubrel.end(),
                                       [& packet](uint16_t packet_id) { return packet_id == packet.packet_id; });
        if (previous_packet == qos2_pending_pubrel.end()) {
            qos2_pending_pubrel.push_back(packet.packet_id);
            session_manager.handle_publish(packet);
        }

    }

}

void BrokerSession::handle_puback(const PubackPacket &packet) {

    auto message = find_if(qos1_pending_puback.begin(), qos1_pending_puback.end(),
                           [&packet](const PublishPacket &p) { return p.packet_id == packet.packet_id; });
    if (message != qos1_pending_puback.end()) {
        qos1_pending_puback.erase(message);
    }

}

void BrokerSession::handle_pubrec(const PubrecPacket &packet) {

    qos2_pending_pubrec.erase(
            std::remove_if(qos2_pending_pubrec.begin(), qos2_pending_pubrec.end(),
                           [&packet](const PublishPacket &p) { return p.packet_id == packet.packet_id; }),
            qos2_pending_pubrec.end()
    );

    auto pubcomp_packet = find_if(qos2_pending_pubcomp.begin(), qos2_pending_pubcomp.end(),
                                  [&packet](uint16_t packet_id) { return packet_id == packet.packet_id; });

    if (pubcomp_packet == qos2_pending_pubcomp.end()) {
        qos2_pending_pubcomp.push_back(packet.packet_id);
    }

}

void BrokerSession::handle_pubrel(const PubrelPacket &packet) {

    qos2_pending_pubrel.erase(
            std::remove_if(qos2_pending_pubrel.begin(), qos2_pending_pubrel.end(),
                           [&packet](uint16_t packet_id) { return packet_id == packet.packet_id; }),
            qos2_pending_pubrel.end()
    );

    PubcompPacket pubcomp;
    pubcomp.packet_id = packet.packet_id;
    packet_manager->send_packet(pubcomp);
}

void BrokerSession::handle_pubcomp(const PubcompPacket &packet) {

    qos2_pending_pubcomp.erase(
            std::remove_if(qos2_pending_pubcomp.begin(), qos2_pending_pubcomp.end(),
                           [&packet](uint16_t packet_id) { return packet_id == packet.packet_id; }),
            qos2_pending_pubcomp.end()
    );

}

void BrokerSession::handle_subscribe(const SubscribePacket &packet) {

    SubackPacket suback;

    suback.packet_id = packet.packet_id;

    for (auto subscription : packet.subscriptions) {

        auto previous_subscription = find_if(subscriptions.begin(), subscriptions.end(),
                                             [&subscription](const Subscription &s) {
                                                 return topic_match(s.topic_filter, subscription.topic_filter);
                                             });
        if (previous_subscription != subscriptions.end()) {
            subscriptions.erase(previous_subscription);
        }

        subscriptions.push_back(subscription);

        SubackPacket::ReturnCode return_code = SubackPacket::ReturnCode::Failure;
        switch (subscription.qos) {
            case QoSType::QoS0:
                return_code = SubackPacket::ReturnCode::SuccessQoS0;
                break;
            case QoSType::QoS1:
                return_code = SubackPacket::ReturnCode::SuccessQoS1;
                break;
            case QoSType::QoS2:
                return_code = SubackPacket::ReturnCode::SuccessQoS2;
                break;
        }
        suback.return_codes.push_back(return_code);
    }

    packet_manager->send_packet(suback);

}

void BrokerSession::handle_unsubscribe(const UnsubscribePacket &packet) {

    UnsubackPacket unsuback;

    unsuback.packet_id = packet.packet_id;

    packet_manager->send_packet(unsuback);

}

void BrokerSession::handle_disconnect(const DisconnectPacket &packet) {
    if (clean_session) {
        session_manager.erase_session(this);
    }
}
