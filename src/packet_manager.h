/**
 * @file packet_manager.h
 *
 * Manage low level network communications.
 *
 * The PacketManager is responsible for sending and receiving MQTT 3.1.1 standard control packets across the network
 * connection.  A PacketManager is installed into every BaseSession and can be moved between sessions to implement
 * session persistance.
 *
 * MQTT control packets received by the PacketManager and events are forwared to a containing session through
 * callbacks.  Session instances control the packet manager by invoking its methods directly.
 */

#pragma once

#include "packet.h"

#include <event2/event.h>
#include <event2/bufferevent.h>

#include <iostream>
#include <vector>
#include <cstddef>
#include <memory>

/**
 * PacketManager class.
 *
 * Manage low level network operations and invoke callbacks on MQTT control packet reception or network event.
 */
class PacketManager {
public:

    /**
     * Enumeration constants for PacketManager events.
     *
     * Events are low level network events or unrecoverable protocol errors.
     *
     */
    enum class EventType {
        NetworkError,
        ProtocolError,
        ConnectionClosed,
        Timeout,
    };

    /**
     * Constructor
     *
     * The PacketManager constructor accepts a pointer to a libevent bufferevent internal structure.  Callbacks are
     * installed for network data received and network events.  A pointer to this PacketManager instance is passed
     * as the user data argument.  It will be used to invoke instance methods from the static callback wrapper.
     *
     * @param bev Pointer to a bufferevent control structure.
     */
    PacketManager(struct bufferevent *bev) : bev(bev) {
        bufferevent_setcb(bev, input_ready, NULL, network_event, this);
        bufferevent_enable(bev, EV_READ);
    }

    /**
     * Destructor
     *
     * Will free the bufferevent pointer.  This call should also close any underlying socket connection provided
     * the libevent flag LEV_OPT_CLOSE_ON_FREE was used to create the bufferevent.
     */
    ~PacketManager() {
        if (bev) {
            bufferevent_free(bev);
            bev = nullptr;
        }
    }

    /**
     * Send a control packet through the network connection.
     *
     * This method is invoked by containing session instances when they want to send a control packet.  The packet
     * will be serialized and transmitted provided the underlying socket connection is not closed.
     */
    void send_packet(const Packet &);

    /**
     * Close the network connection.
     *
     * Explicitly close the network connection maintained by the bufferevent.  This connection should also be closed
     * when the destructor for this instance is run provided the bufferevent was created with the LEV_OPT_CLOSE_ON_FREE
     * flag.
     */
    void close_connection();

    /**
     * Set the packet received callback.
     *
     * This callback will be invoked when a packet is received from the network and deserialized.  The callback will
     * assume ownership of the packet memory.  A pointer to the base packet type is passed and the actual packet can
     * be recovered through a dynamic_cast<>().
     *
     * @param handler Callback function.
     */
    void set_packet_received_handler(std::function<void(std::unique_ptr<Packet>)> handler) {
        packet_received_handler = handler;
    }

    /**
     * Set the network event callback.
     *
     * This callback will be invoked when a low level network or protocol error is detected.  The type of event is
     * indicated by the an EventType enumeration constant passed as an argument to the callback.
     *
     * @param handler Callback function.
     */
    void set_event_handler(std::function<void(EventType)> handler) {
        event_handler = handler;
    }

    /**
     * Return the next available packet id in sequence.
     *
     * @return Next packet id.
     */
    uint16_t next_packet_id() {

        if (++packet_id == 0) {
            ++packet_id;
        }
        return packet_id;
    }

    /**
     * Pointer to the contained libevent bufferevent internal control structure.
     */
    struct bufferevent *bev;

private:

    /**
     * Data receiving method.
     *
     * This instance method is invoked from the static input_ready callback wrapper.  It is run asynchronously
     * whenever data is received from the network connection.  The data will be buffered inside the bufferevent control
     * structure until a complete control packet is received.  At that point the packet will be deserialized and
     * passed to any installed packet_received_handler callback.
     */
    void receive_packet_data();

    /**
     * Libevent callback wrapper.
     *
     * Static method invoked by libevent C library.  The callback method will be passed a pointer to a bufferevent
     * control structure.  This should be the same pointer that was originally passed to the PacketManager constructor.
     * The method will also receive a pointer to the containing PacketManager instance which will be used to invoke the
     * receive_packet_data instance method.
     *
     * @param bev Pointer to a bufferevent
     * @param arg Pointer to user data installed with the callback.  In this case it is a pointer to the containing
     * PacketManager instance.
     */
    static void input_ready(struct bufferevent *bev, void *arg) {
        PacketManager *_this = static_cast<PacketManager *>(arg);
        _this->receive_packet_data();
    }

    /**
     * Network event callback.
     *
     * This instance method is invoked from the static network_event callback wrapper.  It will be passed a set of
     * flags indicating the type of network event that caused the invocation.  The method will then deletegate to any
     * installed event_handler callback passing the event type as the an EventType argument.
     *
     * @param events
     */
    void handle_events(short events);

    /**
     * Libevent callback wrapper.
     *
     * The callback method will be passed a pointer to a bufferevent control structure.  This should be the same
     * pointer that was originally passed to the PacketManager constructor.  The method will also receive a pointer
     * to the containing PacketManager instance which will be used to invoke the handle_events instance method.
     *
     * @param bev    Pointer to a bufferevent control structure.
     * @param events Flags indicating the type of event that caused the callback to be invoked.
     * @param arg    Pointer to the containing PacketManager instance, installed along with the callback wrapper.
     */
    static void network_event(struct bufferevent *bev, short events, void *arg) {
        PacketManager *_this = static_cast<PacketManager *>(arg);
        _this->handle_events(events);
    }

    /**
     * Packet deserialization method.
     *
     * This method will receive a packet_data_t container when the receive_packet_data method has determined
     * that data for a complete packet has been received.  The packet will be deserialized and a reference counted
     * pointer to the instantiated packet will be returned.
     *
     * @param packet_data Reference to a packet_data_t container.
     * @return            Reference counted pointer to Packet.
     */
    std::unique_ptr<Packet> parse_packet_data(const packet_data_t &packet_data);

    /**
     * Packet received callback.
     *
     * Callback installed to be invoked by this PacketManager when an MQTT control packet is received from the network.
     */
    std::function<void(std::unique_ptr<Packet>)> packet_received_handler;

    /**
     * Network event callback.
     *
     * Callback installed to be invoked by this PacketManager when a low level network or protocol error is detected.
     */
    std::function<void(EventType)> event_handler;

    /** Packet id counter. */
    uint16_t packet_id = 0;

    /** State variable used to determine when data for a complete control packet is available. */
    size_t fixed_header_length = 0;

    /** State variable used to determine when data for a complete control packet is available. */
    size_t remaining_length = 0;

};