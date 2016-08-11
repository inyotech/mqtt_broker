#include "session.h"
#include "session_manager.h"

#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>

Session::~Session() {
    std::cout << "~Session\n";
}

void Session::forward_packet(const PublishPacket &packet) {

    if (packet.qos() == 0) {
        packet_manager->send_packet(packet);
    } else if (packet.qos() == 1) {
        PublishPacket packet_to_send(packet);
        packet_to_send.dup(false);
        packet_to_send.retain(false);
        packet_to_send.packet_id = next_packet_id();
        qos1_pending_puback.push_back(packet_to_send);
        packet_manager->send_packet(packet);
    } else if (packet.qos() == 2) {

        PublishPacket packet_to_send(packet);
        packet_to_send.dup(false);
        packet_to_send.retain(false);
        packet_to_send.packet_id = next_packet_id();

        auto previous_packet = find_if(qos2_pending_pubrec.begin(), qos2_pending_pubrec.end(),
                                       [&packet](const PublishPacket &p) { return p.packet_id == packet.packet_id; });
        if (previous_packet == qos2_pending_pubrec.end()) {
            qos2_pending_pubrec.push_back(packet_to_send);
        }

        packet_manager->send_packet(packet);

    }
}

void Session::packet_received(std::unique_ptr<Packet> packet) {

    switch (packet->type) {
        case PacketType::Connect:
            handle_connect(dynamic_cast<const ConnectPacket &>(*packet));
            break;
        case PacketType::Publish:
            handle_publish(dynamic_cast<const PublishPacket &>(*packet));
            break;
        case PacketType::Puback:
            handle_puback(dynamic_cast<const PubackPacket &>(*packet));
            break;
        case PacketType::Pubrec:
            handle_pubrec(dynamic_cast<const PubrecPacket &>(*packet));
            break;
        case PacketType::Pubrel:
            handle_pubrel(dynamic_cast<const PubrelPacket &>(*packet));
            break;
        case PacketType::Pubcomp:
            handle_pubcomp(dynamic_cast<const PubcompPacket &>(*packet));
            break;
        case PacketType::Subscribe:
            handle_subscribe(dynamic_cast<const SubscribePacket &>(*packet));
            break;
        case PacketType::Unsubscribe:
            handle_unsubscribe(dynamic_cast<const UnsubscribePacket &>(*packet));
            break;
        case PacketType::Pingreq:
            handle_pingreq(dynamic_cast<const PingreqPacket &>(*packet));
            break;
        case PacketType::Disconnect:
            handle_disconnect(dynamic_cast<const DisconnectPacket &>(*packet));
            break;
        default:
            break;
    }

    send_pending_message();
}

void Session::resume_session(std::unique_ptr<Session> &session,
                             std::unique_ptr<PacketManager> packet_manager_ptr) {

    std::cout << "restoring session for " << session->client_id << "\n";

    packet_manager_ptr->set_packet_received_handler(
            std::bind(&Session::packet_received, session.get(), std::placeholders::_1));
    session->packet_manager = std::move(packet_manager_ptr);

    ConnackPacket connack;

    connack.session_present(false);
    connack.return_code = ConnackPacket::ReturnCode::Accepted;

    session->packet_manager->send_packet(connack);
}

bool Session::authorize_connection(const ConnectPacket &packet) {
    return true;
}

void Session::send_pending_message()
{

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

void Session::handle_connect(const ConnectPacket &packet) {

    std::cout << "handle " << packet.protocol_name << " connect from " << packet.client_id << "\n";

    if (!authorize_connection(packet)) {
        session_manager.remove_session(this);
        return;
    }

    if (packet.clean_session()) {
        session_manager.remove_session(packet.client_id);
    } else {
        auto previous_session_it = session_manager.find_session(packet.client_id);
        if (previous_session_it != session_manager.sessions.end()) {
            std::unique_ptr<Session> &previous_session_ptr = *previous_session_it;
            resume_session(previous_session_ptr, std::move(packet_manager));
            session_manager.remove_session(this);
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

void Session::handle_publish(const PublishPacket &packet) {

    if (packet.qos() == 0) {

        session_manager.handle_publish(packet);

    } else if (packet.qos() == 1) {

        session_manager.handle_publish(packet);
        PubackPacket puback;
        puback.packet_id = packet.packet_id;
        packet_manager->send_packet(puback);

    } else if (packet.qos() == 2) {

        auto previous_packet = find_if(qos2_pending_pubrel.begin(), qos2_pending_pubrel.end(),
                                       [& packet](uint16_t packet_id) { return packet_id == packet.packet_id; });
        if (previous_packet == qos2_pending_pubrel.end()) {
            qos2_pending_pubrel.push_back(packet.packet_id);
            session_manager.handle_publish(packet);
        }

        PubrecPacket pubrec;
        pubrec.packet_id = packet.packet_id;
        packet_manager->send_packet(pubrec);
    }

}


void Session::handle_puback(const PubackPacket &packet) {

    auto message = find_if(qos1_pending_puback.begin(), qos1_pending_puback.end(),
                           [&packet](const PublishPacket &p) { return p.packet_id == packet.packet_id; });
    if (message != qos1_pending_puback.end()) {
        std::cout << "removing puback' message\n";
        qos1_pending_puback.erase(message);
    }

}

void Session::handle_pubrec(const PubrecPacket &packet) {

    auto sent_packet = find_if(qos2_pending_pubrec.begin(), qos2_pending_pubrec.end(),
                               [&packet](const PublishPacket &p) { return p.packet_id == packet.packet_id; });
    if (sent_packet != qos2_pending_pubrec.end()) {
        qos2_pending_pubrec.erase(sent_packet);
    }
    auto pubrec_packet = find_if(qos2_pending_pubcomp.begin(), qos2_pending_pubcomp.end(),
                                 [&packet](uint16_t packet_id) { return packet_id == packet.packet_id; });

    if (pubrec_packet == qos2_pending_pubcomp.end()) {
        qos2_pending_pubcomp.push_back(packet.packet_id);
    }

    PubrelPacket pubrel;
    pubrel.packet_id = packet.packet_id;
    packet_manager->send_packet(pubrel);
}

void Session::handle_pubrel(const PubrelPacket &packet) {

    auto previous_packet = find_if(qos2_pending_pubrel.begin(), qos2_pending_pubrel.end(),
                                   [&packet](uint16_t packet_id) { return packet_id == packet.packet_id; });
    if (previous_packet != qos2_pending_pubrel.end()) {
        qos2_pending_pubrel.erase(previous_packet);
    }

    PubcompPacket pubcomp;
    pubcomp.packet_id = packet.packet_id;
    packet_manager->send_packet(pubcomp);
}

void Session::handle_pubcomp(const PubcompPacket &packet) {

    auto previous_packet = find_if(qos2_pending_pubcomp.begin(), qos2_pending_pubcomp.end(),
                                   [&packet](uint16_t packet_id) { return packet_id == packet.packet_id; });
    if (previous_packet != qos2_pending_pubcomp.end()) {
        qos2_pending_pubcomp.erase(previous_packet);
    }
}

void Session::handle_subscribe(const SubscribePacket &packet) {

    SubackPacket suback;

    suback.packet_id = packet.packet_id;

    for (auto subscription : packet.subscriptions) {
        std::cout << "subscribing to topic " << std::string(subscription.topic_filter) << " qos "
                  << static_cast<int>(subscription.qos)
                  << "\n";

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
            case 0:
                return_code = SubackPacket::ReturnCode::SuccessQoS0;
                break;
            case 1:
                return_code = SubackPacket::ReturnCode::SuccessQoS1;
                break;
            case 2:
                return_code = SubackPacket::ReturnCode::SuccessQoS2;
                break;
        }
        suback.return_codes.push_back(return_code);
    }

    packet_manager->send_packet(suback);

}

void Session::handle_unsubscribe(const UnsubscribePacket &packet) {

    UnsubackPacket unsuback;

    for (auto topic : packet.topics) {
        std::cout << "unsubscribe " << topic << "\n";
    }

    unsuback.packet_id = packet.packet_id;

    packet_manager->send_packet(unsuback);

}

void Session::handle_pingreq(const PingreqPacket &packet) {

    PingrespPacket pingresp;

    packet_manager->send_packet(pingresp);

}

void Session::handle_disconnect(const DisconnectPacket &packet) {

    std::cout << "handle disconnect\n";

}
