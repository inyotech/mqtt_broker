//
// Created by Scott Brumbaugh on 8/3/16.
//

#pragma once

#include <string>
#include <cstdint>
#include <cstddef>
#include <vector>

class PacketDataWriter
{
public:
    PacketDataWriter(std::vector<uint8_t> & packet_data) : packet_data(packet_data) {
        packet_data.resize(0);
    }

    void write_remaining_length(size_t length);
    void write_byte(uint8_t byte);
    void write_uint16(uint16_t word);
    void write_string(const std::string & s);
    void write_bytes(const std::vector<uint8_t> & b);
    std::vector<uint8_t> & packet_data;
};

class PacketDataReader
{

public:

    PacketDataReader(const std::vector<uint8_t> & packet_data) : offset(0), packet_data(packet_data) {}

    bool has_remaining_length();

    size_t read_remaining_length();
    uint8_t read_byte();
    uint16_t read_uint16();
    std::string read_string();
    std::vector<uint8_t> read_bytes();
    std::vector<uint8_t> read_bytes(size_t n);
    bool empty();
    size_t get_offset() { return offset; }
    std::vector<uint8_t> get_packet_data() { return packet_data; }

private:
    size_t offset;
    const std::vector<uint8_t> & packet_data;
};
