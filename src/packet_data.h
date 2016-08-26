/**
 * @file packet_data.h
 *
 * Utility classes supporting serialization/deserialization of MQTT control packets.
 *
 *
 */

#pragma once

#include <string>
#include <cstdint>
#include <cstddef>
#include <vector>

typedef std::vector<uint8_t> packet_data_t;

class PacketDataWriter
{
public:
    PacketDataWriter(packet_data_t & packet_data) : packet_data(packet_data) {
        packet_data.resize(0);
    }

    void write_remaining_length(size_t length);
    void write_byte(uint8_t byte);
    void write_uint16(uint16_t word);
    void write_string(const std::string & s);
    void write_bytes(const packet_data_t & b);
    packet_data_t & packet_data;
};

class PacketDataReader
{

public:

    PacketDataReader(const packet_data_t & packet_data) : offset(0), packet_data(packet_data) {}

    bool has_remaining_length();

    size_t read_remaining_length();
    uint8_t read_byte();
    uint16_t read_uint16();
    std::string read_string();
    packet_data_t read_bytes();
    packet_data_t read_bytes(size_t n);
    bool empty();
    size_t get_offset() { return offset; }
    packet_data_t get_packet_data() { return packet_data; }

private:
    size_t offset;
    const packet_data_t & packet_data;
};
