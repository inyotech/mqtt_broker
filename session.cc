#include "session.h"

#include <iostream>

Session::~Session() {
    std::cout << "deleting session\n";
}

void Session::packet_received(std::unique_ptr<Packet> packet) {

    switch (packet->type) {
        case PacketType::Connect:
            handle_connect(dynamic_cast<const ConnectPacket &>(*packet));
            break;
        case PacketType::Publish:
            handle_publish(dynamic_cast<const PublishPacket &>(*packet));
            break;
        case PacketType::Pubrel:
            handle_pubrel(dynamic_cast<const PubrelPacket &>(*packet));
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

void Session::handle_connect(const ConnectPacket & packet) {

    ConnackPacket connack;

    connack.session_present(false);
    connack.return_code = ConnackPacket::ReturnCode::Accepted;

    packet_manager.send_packet(connack);
}

void Session::handle_publish(const PublishPacket & packet) {

    std::cout << "handle publish\n";

    if (packet.qos() == 1) {
        PubackPacket puback;
        puback.packet_id = packet.packet_id;
        std::cout << "puback " << puback.packet_id << "\n";
        packet_manager.send_packet(puback);
    } else if (packet.qos() == 2) {
        PubrecPacket pubrec;
        pubrec.packet_id = packet.packet_id;
        std::cout << "puback " << pubrec.packet_id << "\n";
        packet_manager.send_packet(pubrec);
    }

}

void Session::handle_pubrel(const PubrelPacket & packet) {

    PubcompPacket pubcomp;

    pubcomp.packet_id = packet.packet_id;
    std::cout << "pubcomp " << pubcomp.packet_id << "\n";
    packet_manager.send_packet(pubcomp);
}

void Session::handle_subscribe(const SubscribePacket & packet) {

    std::cout << "handle subscribe\n";

    SubackPacket suback;

    suback.packet_id = packet.packet_id;

    for(auto subscription : packet.subscriptions) {
        std::cout << "topic " << subscription.topic << " " << static_cast<int>(subscription.qos) << "\n";
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

    packet_manager.send_packet(suback);

}

void Session::handle_unsubscribe(const UnsubscribePacket & packet) {

    std::cout << "handle unsubscribe\n";

    UnsubackPacket unsuback;

    for (auto topic : packet.topics) {
        std::cout << "unsubscribe " << topic << "\n";
    }

    unsuback.packet_id = packet.packet_id;

    packet_manager.send_packet(unsuback);

}

void Session::handle_pingreq(const PingreqPacket & packet) {

    std::cout << "handle pingreq\n";

    PingrespPacket pingresp;

    packet_manager.send_packet(pingresp);

}

void Session::handle_disconnect(const DisconnectPacket & packet) {

    std::cout << "handle disconnect\n";

}