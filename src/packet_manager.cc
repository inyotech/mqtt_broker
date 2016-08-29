/**
 * @file packet_manager.cc
 */

#include "packet_manager.h"
#include "packet.h"

#include <event2/buffer.h>
#include <evdns.h>

#include <cstring>

void PacketManager::receive_packet_data() {

    struct evbuffer *input = bufferevent_get_input(bev);

    while (evbuffer_get_length(input) != 0) {

        size_t available = evbuffer_get_length(input);

        if (available < 2) {
            return;
        }

        if (fixed_header_length == 0) {

            size_t peek_size = std::min<uint8_t>(static_cast<uint8_t>(available), 5);

            std::vector<uint8_t> peek_buffer(peek_size);
            evbuffer_copyout(input, &peek_buffer[0], peek_size);

            PacketDataReader reader(peek_buffer);
            reader.read_byte();
            if (!reader.has_remaining_length()) {
                if (peek_size == 5) {
                    if (event_handler) {
                        event_handler(EventType::ProtocolError);
                    }
                    evbuffer_drain(input, peek_size);
                }
                return;
            }

            remaining_length = reader.read_remaining_length();
            fixed_header_length = reader.get_offset();

        }

        size_t packet_size = fixed_header_length + remaining_length;

        if (available < packet_size) {
            return;
        }

        packet_data_t packet_data(packet_size);
        evbuffer_remove(input, &packet_data[0], packet_size);

        fixed_header_length = 0;
        remaining_length = 0;

        std::unique_ptr<Packet> packet = parse_packet_data(packet_data);

        if (packet && packet_received_handler) {
            packet_received_handler(std::move(packet));
        }
    }
}

std::unique_ptr<Packet> PacketManager::parse_packet_data(const std::vector<uint8_t> &packet_data) {

    PacketType type = static_cast<PacketType>(packet_data[0] >> 4);

    std::unique_ptr<Packet> packet;

    try {
        switch (type) {
            case PacketType::Connect:
                packet = std::unique_ptr<ConnectPacket>(new ConnectPacket(packet_data));
                break;
            case PacketType::Connack:
                packet = std::unique_ptr<ConnackPacket>(new ConnackPacket(packet_data));
                break;
            case PacketType::Publish:
                packet = std::unique_ptr<PublishPacket>(new PublishPacket(packet_data));
                break;
            case PacketType::Puback:
                packet = std::unique_ptr<PubackPacket>(new PubackPacket(packet_data));
                break;
            case PacketType::Pubrec:
                packet = std::unique_ptr<PubrecPacket>(new PubrecPacket(packet_data));
                break;
            case PacketType::Pubrel:
                packet = std::unique_ptr<PubrelPacket>(new PubrelPacket(packet_data));
                break;
            case PacketType::Pubcomp:
                packet = std::unique_ptr<PubcompPacket>(new PubcompPacket(packet_data));
                break;
            case PacketType::Subscribe:
                packet = std::unique_ptr<SubscribePacket>(new SubscribePacket(packet_data));
                break;
            case PacketType::Suback:
                packet = std::unique_ptr<SubackPacket>(new SubackPacket(packet_data));
                break;
            case PacketType::Unsubscribe:
                packet = std::unique_ptr<UnsubscribePacket>(new UnsubscribePacket(packet_data));
                break;
            case PacketType::Unsuback:
                packet = std::unique_ptr<UnsubackPacket>(new UnsubackPacket(packet_data));
                break;
            case PacketType::Pingreq:
                packet = std::unique_ptr<PingreqPacket>(new PingreqPacket(packet_data));
                break;
            case PacketType::Pingresp:
                packet = std::unique_ptr<PingrespPacket>(new PingrespPacket(packet_data));
                break;
            case PacketType::Disconnect:
                packet = std::unique_ptr<DisconnectPacket>(new DisconnectPacket(packet_data));
                break;
        }
    } catch (std::exception &e) {
        if (event_handler) {
            event_handler(EventType::ProtocolError);
        }
    }
    return packet;
}

void PacketManager::send_packet(const Packet &packet) {
    std::vector<uint8_t> packet_data = packet.serialize();
    if (bev) {
        bufferevent_write(bev, &packet_data[0], packet_data.size());
    } else {
        std::cout << "not writing to closed bev\n";
    }
}

void PacketManager::close_connection() {
    if (bev) {
        evutil_socket_t fd = bufferevent_getfd(bev);
        evutil_closesocket(fd);
        bufferevent_free(bev);
        bev = nullptr;
    }
}

void PacketManager::handle_events(short events) {

    if (events & BEV_EVENT_EOF) {
        if (event_handler) {
            event_handler(EventType::ConnectionClosed);
        }
    } else if (events & BEV_EVENT_ERROR) {
        if (event_handler) {
            event_handler(EventType::NetworkError);
        }
    } else if (events & BEV_EVENT_TIMEOUT) {
        if (event_handler) {
            event_handler(EventType::Timeout);
        }
    }
}
