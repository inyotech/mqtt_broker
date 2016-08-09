//
// Created by Scott Brumbaugh on 8/3/16.
//

#include "packet_manager.h"
#include "packet.h"

#include <event2/buffer.h>

void PacketManager::receive_packet_data(struct bufferevent * bev) {

    struct evbuffer *input = bufferevent_get_input(bev);

    size_t available = evbuffer_get_length(input);

    if (available < 2) {
        return;
    }

    if (fixed_header_length == 0) {

        size_t peek_size = std::min<uint8_t>(static_cast<uint8_t>(available), 5);
        size_t peek_offset = 1;

        uint8_t peek_buffer[peek_size];
        evbuffer_copyout(input, peek_buffer, peek_size);

        int multiplier = 1;
        size_t value = 0;

        do {
            uint8_t encoded_byte = peek_buffer[peek_offset++];
            value += (encoded_byte & 0x7F) * multiplier;

            if ((encoded_byte & 0x80) == 0) {
                break;
            }

            multiplier <<= 7;

            if (peek_offset > (available - 1)) {
                return;
            } else if (peek_offset > 4) {
                throw std::exception();
            }
        } while (1);

        fixed_header_length = peek_offset;
        remaining_length = value;
    }

    size_t packet_size = fixed_header_length + remaining_length;

    if (available < packet_size) {
        return;
    }

    uint8_t command;
    evbuffer_copyout(input, &command, 1);
    evbuffer_drain(input, fixed_header_length);

    packet_data_in.resize(remaining_length);
    evbuffer_remove(input, &packet_data_in[0], remaining_length);

    std::unique_ptr<Packet> packet = parse_packet_data(command);

    fixed_header_length = 0;
    remaining_length = 0;
    packet_data_in.clear();

    if (packet_received_handler) {
        packet_received_handler(std::move(packet));
    }

}

std::unique_ptr<Packet> PacketManager::parse_packet_data(uint8_t command) {

    std::unique_ptr<Packet> packet;

    switch(static_cast<PacketType>(command >> 4)) {
        case PacketType::Connect:
            packet = std::unique_ptr<ConnectPacket>(new ConnectPacket(command, packet_data_in));
            break;
        case PacketType::Publish:
            packet = std::unique_ptr<PublishPacket>(new PublishPacket(command, packet_data_in));
            break;
        case PacketType::Puback:
            packet = std::unique_ptr<PubackPacket>(new PubackPacket(command, packet_data_in));
            break;
        case PacketType::Pubrec:
            packet = std::unique_ptr<PubrecPacket>(new PubrecPacket(command, packet_data_in));
            break;
        case PacketType::Pubrel:
            packet = std::unique_ptr<PubrelPacket>(new PubrelPacket(command, packet_data_in));
            break;
        case PacketType::Pubcomp:
            packet = std::unique_ptr<PubcompPacket>(new PubcompPacket(command, packet_data_in));
            break;
        case PacketType::Subscribe:
            packet = std::unique_ptr<SubscribePacket>(new SubscribePacket(command, packet_data_in));
            break;
        case PacketType::Pingreq:
            packet = std::unique_ptr<PingreqPacket>(new PingreqPacket(command, packet_data_in));
            break;
        case PacketType::Disconnect:
            packet = std::unique_ptr<DisconnectPacket>(new DisconnectPacket(command, packet_data_in));
            break;
        default:
            break;
    }

    return packet;
}

void PacketManager::send_packet(const Packet & packet) {
    std::vector<uint8_t> packet_data = packet.serialize();
    if (bev) {
        bufferevent_write(bev, &packet_data[0], packet_data.size());
    } else {
        std::cout << "not writing to closed bev\n";
    }
}

void PacketManager::handle_other_events(short events)
{
    if (events & BEV_EVENT_EOF) {
        std::cout << "Connection closed\n";
        bufferevent_free(bev);
        bev = nullptr;
    } else if (events & BEV_EVENT_ERROR) {
        std::cout << "Got an error on the connection: " << std::strerror(errno) << "\n";
        bufferevent_free(bev);
        bev = nullptr;
    }
}
