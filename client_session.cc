#include "client_session.h"

ClientSession::~ClientSession() {
    std::cout << "~ClientSession\n";
}

void ClientSession::packet_received(std::unique_ptr<Packet> packet) {

    switch (packet->type) {
        case PacketType::Connack:
            handle_connack(dynamic_cast<const ConnackPacket &>(*packet));
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
        case PacketType::Unsuback:
            handle_unsuback(dynamic_cast<const UnsubackPacket &>(*packet));
            break;
        case PacketType::Pingresp:
            handle_pingresp(dynamic_cast<const PingrespPacket &>(*packet));
            break;
        default:
            break;
    }

}


void ClientSession::send_connect(const ConnectPacket & packet)
{
    std::cout << "send connect\n";
    packet_manager->send_packet(packet);
}

void ClientSession::send_publish(const PublishPacket & packet)
{
    std::cout << "send publish\n";
    packet_manager->send_packet(packet);

}

void ClientSession::send_subscribe(const SubscribePacket & packet)
{

}

void ClientSession::send_unsubscribe(const UnsubackPacket & packet)
{

}

void ClientSession::send_disconnect(const DisconnectPacket & packet)
{
    std::cout << "send disconnect\n";
    packet_manager->send_packet(packet);
}

void ClientSession::handle_connack(const ConnackPacket & packet) {
    std::cout << "handle connack\n";
}

void ClientSession::handle_publish(const PublishPacket & packet) {

}

void ClientSession::handle_puback(const PubackPacket &packet) {
    std::cout << "handle puback\n";
}

void ClientSession::handle_pubrec(const PubrecPacket &packet) {

}

void ClientSession::handle_pubrel(const PubrelPacket &packet) {

}

void ClientSession::handle_pubcomp(const PubcompPacket &packet) {

}

void ClientSession::handle_unsuback(const UnsubackPacket &packet) {

}

void ClientSession::handle_pingresp(const PingrespPacket &packet) {

    std::cout << "ping response\n";

}

