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
        qos1_sent.push_back(packet_to_send);
        packet_manager->send_packet(packet);
    } else if (packet.qos() == 2) {
        // only forward the packet 1 time, check if we have already forwarded this packet id
        PublishPacket packet_to_send(packet);
        packet_to_send.dup(false);
        packet_to_send.retain(false);
        packet_to_send.packet_id = next_packet_id();
        qos2_sent.push_back(packet_to_send);
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
}

void Session::restore_session(Session *session, std::unique_ptr<PacketManager> packet_manager_ptr) {

    std::cout << "restoring session for " << session->client_id << "\n";

    packet_manager_ptr->set_packet_received_handler(
            std::bind(&Session::packet_received, session, std::placeholders::_1));
    session->packet_manager = std::move(packet_manager_ptr);

    session->session_manager.remove_session(this);

    ConnackPacket connack;

    connack.session_present(false);
    connack.return_code = ConnackPacket::ReturnCode::Accepted;

    session->packet_manager->send_packet(connack);
}

void Session::handle_connect(const ConnectPacket &packet) {

    std::cout << "handle " << packet.protocol_name << " connect from " << packet.client_id << "\n";

    Session *previous_session = session_manager.find_session(packet.client_id);
    if (previous_session) {
        restore_session(previous_session, std::move(packet_manager));
        return;
    } else {
        client_id = packet.client_id;
        ConnackPacket connack;

        connack.session_present(false);
        connack.return_code = ConnackPacket::ReturnCode::Accepted;

        packet_manager->send_packet(connack);
    }

}

void Session::handle_publish(const PublishPacket &packet) {

    if (packet.qos() == 0) {

        session_manager.handle_publish(packet);

    } else if (packet.qos() == 1) {

        session_manager.handle_publish(packet);
        PubackPacket puback;
        puback.packet_id = packet.packet_id;
        std::cout << "puback " << puback.packet_id << "\n";
        packet_manager->send_packet(puback);

    } else if (packet.qos() == 2) {

        auto previous_packet = find_if(qos2_received.begin(), qos2_received.end(),
                                       [& packet](uint16_t packet_id) { return packet_id == packet.packet_id; });
        if (previous_packet == qos2_received.end()) {
            qos2_received.push_back(packet.packet_id);
            session_manager.handle_publish(packet);
        }

        PubrecPacket pubrec;
        pubrec.packet_id = packet.packet_id;
        std::cout << "puback " << pubrec.packet_id << "\n";
        packet_manager->send_packet(pubrec);
    }

}


void Session::handle_puback(const PubackPacket &packet) {

    auto message = find_if(qos1_sent.begin(), qos1_sent.end(),
                           [&packet](const PublishPacket &p) { return p.packet_id == packet.packet_id; });
    if (message != qos1_sent.end()) {
        std::cout << "removing puback' message\n";
        qos1_sent.erase(message);
    }

}

void Session::handle_pubrec(const PubrecPacket &packet) {

    auto sent_packet = find_if(qos2_sent.begin(), qos2_sent.end(),
                               [&packet](const PublishPacket &p) { return p.packet_id == packet.packet_id; });
    if (sent_packet != qos2_sent.end()) {
        qos2_sent.erase(sent_packet);
    }
    auto pubrec_packet = find_if(qos2_pubrel.begin(), qos2_pubrel.end(),
                                 [&packet](uint16_t packet_id) { return packet_id == packet.packet_id; });

    if (pubrec_packet == qos2_pubrel.end()) {
        qos2_pubrel.push_back(packet.packet_id);
    }

    PubrelPacket pubrel;
    pubrel.packet_id = packet.packet_id;
    packet_manager->send_packet(pubrel);
}

void Session::handle_pubrel(const PubrelPacket &packet) {

    auto previous_packet = find_if(qos2_received.begin(), qos2_received.end(),
                                   [&packet](uint16_t packet_id) { return packet_id == packet.packet_id; });
    if (previous_packet != qos2_received.end()) {
        qos2_received.erase(previous_packet);
    }

    PubcompPacket pubcomp;
    pubcomp.packet_id = packet.packet_id;
    packet_manager->send_packet(pubcomp);
}

void Session::handle_pubcomp(const PubcompPacket &packet) {

    auto previous_packet = find_if(qos2_pubrel.begin(), qos2_pubrel.end(),
                                   [&packet](uint16_t packet_id) { return packet_id == packet.packet_id; });
    if (previous_packet != qos2_pubrel.end()) {
        qos2_pubrel.erase(previous_packet);
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