//
// Created by Scott Brumbaugh on 8/3/16.
//

#pragma once

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

    std::vector<uint8_t> & packet_data;
};

class PacketDataReader
{

public:

    PacketDataReader(const std::vector<uint8_t> & packet_data) : offset(0), packet_data(packet_data) {}

    uint8_t read_byte();
    uint16_t read_uint16();
    std::string read_string();
    std::vector<uint8_t> read_bytes();
    std::vector<uint8_t> read_bytes(size_t n);
    bool empty();

    size_t offset;
    const std::vector<uint8_t> & packet_data;
};
