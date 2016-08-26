/**
 * @file packet_data.h
 *
 * Utility classes supporting serialization/deserialization of MQTT control packets.
 */

#pragma once

#include <string>
#include <cstdint>
#include <cstddef>
#include <vector>

/** Typedef for packet data container. */
typedef std::vector<uint8_t> packet_data_t;

/**
 * Serialization class.
 *
 * Methods are provided to write native types to the MQTT 3.1.1 standard wire format.
 */
class PacketDataWriter
{
public:

    /**
     * Constructor
     *
     * Accepts a reference to a packet_data_t container.  This container will be filled with serialized data and
     * can be sent directly through the network connection.
     *
     * @param packet_data A reference to a packet_data_t container.
     */
    PacketDataWriter(packet_data_t & packet_data) : packet_data(packet_data) {
        packet_data.resize(0);
    }

    /**
     * Write the integer length to the container using the MQTT 3.1.1 remaining length encoding scheme.
     *
     * @param length The value to encode.
     */
    void write_remaining_length(size_t length);

    /** Write a byte to the packet_data_t container. */
    void write_byte(uint8_t byte);

    /** Write a 16 bit value to the packet_data_t container. */
    void write_uint16(uint16_t word);

    /** Write a UTF-8 character string to the packet_data_t container. */
    void write_string(const std::string & s);

    /** Copy the contents of a packet_data_t container into this instance's container. */
    void write_bytes(const packet_data_t & b);

private:

    /** A reference to the packet_data_t container. */
    packet_data_t & packet_data;
};

/**
 * Deserialization class.
 *
 * Methods are provided to read native types from the wire encoded control packets received over the network
 * connection.  MQTT 3.1.1 standard decoding methods are implemented.
 */
class PacketDataReader
{

public:

    /**
     * Constructor
     *
     * Accepts a reference to a packet_data_t container that contains data received directly over a network connection.
     * The class also contains a current offset pointer that is initialized to point to the beginning of the container.
     * Each data read will advance the offset pointer forward to the next item in the container.
     *
     * @param packet_data A reference to a packet_data_t container.
     */
    PacketDataReader(const packet_data_t & packet_data) : offset(0), packet_data(packet_data) {}

    /**
     * Is a remaing lenght value present.
     *
     * Primitive function indicating a valid remaining_length value can be read from the current position in the
     * packet_data_t container.  The remaining length value is encoded in a variable sequence from 1 to 4 bytes.
     *
     * @return valid remaining length.
     */
    bool has_remaining_length();

    /**
     * Read the remaining length value from the packet_data_t container.
     *
     * @return integer.
     */
    size_t read_remaining_length();

    /**
     * Read a single byte from the packet_data_t container.
     *
     * @return byte.
     */
    uint8_t read_byte();

    /**
     * Read a 16 bit value from the packet_data_t container.
     *
     * @return 16 bit integer.
     */
    uint16_t read_uint16();

    /**
     * Read a UTF-8 encoded string from the packet_data_t container.
     *
     *  @return character string.
     */
    std::string read_string();

    /**
     * Read a byte sequence from the packet_data_t container.
     *
     * The sequence is assumed to be encoded according to the MQTT 3.1.1 standard wire format for a byte sequence.
     * The sequence length is encoded along with the data.
     *
     * @return byte seqence.
     */
    std::vector<uint8_t> read_bytes();

    /**
     * Read a byte sequence from the packet_data_t contianer.
     *
     * The number of bytes read is passed as an argument to the method.
     *
     * @param len Lenght of the sequence to read.
     * @return    Byte sequence
     */
    std::vector<uint8_t> read_bytes(size_t len);

    /**
     * Is the packet_data_t container empty.
     *
     * @return empty.
     */
    bool empty();

    /**
     * Return the current packet_data_t container offset pointer.
     *
     * @return integer.
     */
    size_t get_offset() { return offset; }

    /**
     * Get a reference to the packet_data_t container.
     *
     * @return Reference to packet_data_t container.
     */
    const packet_data_t & get_packet_data() { return packet_data; }

private:

    /** Current packet_data_t container offset pointer */
    size_t offset;

    /** Packet data container. */
    const packet_data_t & packet_data;
};
