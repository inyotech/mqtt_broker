//
// Created by Scott Brumbaugh on 8/3/16.
//

#include "packet_data.h"

#include <string>

void PacketDataWriter::write_remaining_length(size_t length)
{
    do {
        uint8_t encoded_byte = length % 0x80;
        length /= 0x80;
        if (length > 0) {
            encoded_byte |= 0x80;
        }
        packet_data.push_back(encoded_byte);
    } while(length > 0);
}

void PacketDataWriter::write_byte(uint8_t byte)
{
    packet_data.push_back(byte);
}

void PacketDataWriter::write_uint16(uint16_t word)
{
    packet_data.push_back((word >> 8) & 0xFF);
    packet_data.push_back(word & 0xFF);
}

uint8_t PacketDataReader::read_byte()
{
    if (offset > packet_data.size()) {
        throw std::exception();
    }
    return packet_data[offset++];
}

uint16_t PacketDataReader::read_uint16()
{
    if (offset + 2 > packet_data.size()) {
        throw std::exception();
    }
    uint8_t msb = packet_data[offset++];
    uint8_t lsb = packet_data[offset++];
    return (msb << 8) + lsb;
}

std::string PacketDataReader::read_string()
{
    uint16_t len = read_uint16();
    if (offset + len > packet_data.size()) {
        throw std::exception();
    }
    std::string s(&packet_data[offset], &packet_data[offset+len]);
    offset += len;
    return s;
}

std::vector<uint8_t> PacketDataReader::read_bytes() {
    uint16_t len = read_uint16();
    if (offset + len > packet_data.size()) {
        throw std::exception();
    }
    std::vector<uint8_t> v(&packet_data[offset], &packet_data[offset+len]);
    offset += len;
    return v;
}

std::vector<uint8_t> PacketDataReader::read_bytes(size_t len) {
    if (offset + len > packet_data.size()) {
        throw std::exception();
    }
    std::vector<uint8_t> v(&packet_data[offset], &packet_data[offset+len]);
    offset += len;
    return v;
}

bool PacketDataReader::empty() {
    return offset == packet_data.size();
}