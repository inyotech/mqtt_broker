//
// Created by Scott Brumbaugh on 8/23/16.
//

#include "session_base.h"

void SessionBase::packet_received(std::unique_ptr<Packet> packet)
{

    switch (packet->type) {
        case PacketType::Connect:
            handle_connect(dynamic_cast<const ConnectPacket &>(*packet));
            break;
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
        case PacketType::Subscribe:
            handle_subscribe(dynamic_cast<const SubscribePacket &>(*packet));
            break;
        case PacketType::Suback:
            handle_suback(dynamic_cast<const SubackPacket &>(*packet));
            break;
        case PacketType::Unsubscribe:
            handle_unsubscribe(dynamic_cast<const UnsubscribePacket &>(*packet));
            break;
        case PacketType::Unsuback:
            handle_unsuback(dynamic_cast<const UnsubackPacket &>(*packet));
            break;
        case PacketType::Pingreq:
            handle_pingreq(dynamic_cast<const PingreqPacket &>(*packet));
            break;
        case PacketType::Pingresp:
            handle_pingresp(dynamic_cast<const PingrespPacket &>(*packet));
            break;
        case PacketType::Disconnect:
            handle_disconnect(dynamic_cast<const DisconnectPacket &>(*packet));
            break;
        default:
            break;
    }

}

void SessionBase::handle_connect(const ConnectPacket &) {
    throw std::exception();
}

void SessionBase::handle_connack(const ConnackPacket &) {
    throw std::exception();
}

void SessionBase::handle_publish(const PublishPacket &) {
    throw std::exception();
}

void SessionBase::handle_puback(const PubackPacket &) {
    throw std::exception();
}

void SessionBase::handle_pubrec(const PubrecPacket &) {
    throw std::exception();
}

void SessionBase::handle_pubrel(const PubrelPacket &) {
    throw std::exception();
}

void SessionBase::handle_pubcomp(const PubcompPacket &) {
    throw std::exception();
}

void SessionBase::handle_subscribe(const SubscribePacket &) {
    throw std::exception();
}

void SessionBase::handle_suback(const SubackPacket &) {
    throw std::exception();
}

void SessionBase::handle_unsubscribe(const UnsubscribePacket &) {
    throw std::exception();
}

void SessionBase::handle_unsuback(const UnsubackPacket &) {
    throw std::exception();
}

void SessionBase::handle_pingreq(const PingreqPacket &) {
    throw std::exception();
}

void SessionBase::handle_pingresp(const PingrespPacket &) {
    throw std::exception();
}

void SessionBase::handle_disconnect(const DisconnectPacket &) {
    throw std::exception();
}
