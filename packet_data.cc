//
// Created by Scott Brumbaugh on 8/3/16.
//

#include "packet_data.h"

#include <iostream>

void PacketDataWriter::write_remaining_length(size_t length) {

    if (length > 127 + 128*127 + 128*128*127 + 128*128*128*127) {
        throw std::exception();
    }

    do {
        uint8_t encoded_byte = length % 0x80;
        length >>= 7;
        if (length > 0) {
            encoded_byte |= 0x80;
        }
        packet_data.push_back(encoded_byte);
    } while (length > 0);
}

void PacketDataWriter::write_byte(uint8_t byte) {
    packet_data.push_back(byte);
}

void PacketDataWriter::write_uint16(uint16_t word) {
    packet_data.push_back((word >> 8) & 0xFF);
    packet_data.push_back(word & 0xFF);
}

void PacketDataWriter::write_string(const std::string &s) {
    write_uint16(s.size());
    std::copy(s.begin(), s.end(), std::back_inserter(packet_data));
}

void PacketDataWriter::write_bytes(const std::vector<uint8_t> &b) {
    write_uint16(b.size());
    std::copy(b.begin(), b.end(), std::back_inserter(packet_data));
}

bool PacketDataReader::has_remaining_length() {

    size_t remaining = std::min<size_t>(packet_data.size() - offset, 4);

    for (int i=offset; i<offset+remaining; i++) {
        if ((packet_data[i] & 0x80) == 0) {
            return true;
        }
    }
    return false;

}

size_t PacketDataReader::read_remaining_length() {

    size_t starting_offset = offset;
    size_t value = 0;
    int multiplier = 1;

    do {
        uint8_t encoded_byte = packet_data[offset++];
        value += (encoded_byte & 0x7F) * multiplier;

        if ((encoded_byte & 0x80) == 0) {
            break;
        }

        multiplier <<= 7;

        if (offset - starting_offset == 4) {
            throw std::exception();
        }

    } while (1);

    return value;
}

uint8_t PacketDataReader::read_byte() {
    if (offset > packet_data.size()) {
        throw std::exception();
    }
    return packet_data[offset++];
}


uint16_t PacketDataReader::read_uint16() {
    if (offset + 2 > packet_data.size()) {
        throw std::exception();
    }
    uint8_t msb = packet_data[offset++];
    uint8_t lsb = packet_data[offset++];
    return (msb << 8) + lsb;
}

std::string PacketDataReader::read_string() {
    uint16_t len = read_uint16();
    if (offset + len > packet_data.size()) {
        throw std::exception();
    }
    std::string s(&packet_data[offset], &packet_data[offset + len]);
    offset += len;
    return s;
}

std::vector<uint8_t> PacketDataReader::read_bytes() {
    uint16_t len = read_uint16();
    if (offset + len > packet_data.size()) {
        throw std::exception();
    }
    std::vector<uint8_t> v(&packet_data[offset], &packet_data[offset + len]);
    offset += len;
    return v;
}

std::vector<uint8_t> PacketDataReader::read_bytes(size_t len) {
    if (offset + len > packet_data.size()) {
        throw std::exception();
    }
    std::vector<uint8_t> v(&packet_data[offset], &packet_data[offset + len]);
    offset += len;
    return v;
}

bool PacketDataReader::empty() {
    return offset == packet_data.size();
}