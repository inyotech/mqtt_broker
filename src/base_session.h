/**
 * @file base_session.h
 *
 * Base class for MQTT sessions.  The MQTT specification requires that the client and server both maintain session
 * state while connection.  The broker is also required to resume the state of a client re-connecting with the same
 * client id.
 *
 * This class add facilities for persistence and resumption of session state.
 */

#pragma once

#include "packet.h"
#include "packet_manager.h"

#include <event2/bufferevent.h>

#include <string>

/**
 * Base session class
 *
 * Will maintain all session attributes and provide default handlers for received control packets.  Specialized
 * Session classes will override the required control packet handlers.
 *
 * Each BaseSession composes a PacketManager instance that in installable on session resumption.
 */
class BaseSession {

public:

    /**
     * Constructor
     *
     * A pointer to a libevent bufferevent control structure is passed as an argument, this pointer is forwarded to a
     * newly instantiated PacketManager that will handle all network related functions.  Install callbacks that the
     * PacketManager will use to communicate low level protocol and network errors, and received control packets.
     *
     * This session can persist in memory after a connection is closed.  If a connection is received from the same
     * client that session will be abandoned and the PacketManager instance from that session will be installed into
     * the saved session. This is how session persistence is implemented.
     *
     * @param bev A pointer to a bufferevent internal control structure.
     */
    BaseSession(struct bufferevent *bev) : packet_manager(new PacketManager(bev)) {
        packet_manager->set_packet_received_handler(
                std::bind(&BaseSession::packet_received, this, std::placeholders::_1));
        packet_manager->set_event_handler(std::bind(&BaseSession::packet_manager_event, this, std::placeholders::_1));
    }

    /**
     * Desctructor
     *
     * Virtual so the desctructor for derived classes will be called.
     */
    virtual ~BaseSession() {}

    std::string client_id;

    bool clean_session;

    /**
     * PacketManager callback.
     *
     * Invoked by the compsed PacketManager when it receives a complete control packet.  The default handler methods
     * will be passes a reference to this packet.  Memory occupied by the packet will be freed by standard C++
     * std::unique_ptr rules.  It is the responsibility of subclasses to maintain the packet lifetime.
     *
     * @param packet Reference counted pointer to a packet.
     */
    virtual void packet_received(std::unique_ptr<Packet> packet);

    /**
     * PacketManager callback.
     *
     * Invoked by the composed PacketManager when it detects a low level protocol or network error.  The default action
     * performed in this method is to close the network connection.
     *
     * @param event The type of event detected.
     */
    virtual void packet_manager_event(PacketManager::EventType event);

    /**
     * Handle a received ConnectPacket.
     *
     * The default action is to throw an exception.  Subclasses should override this method.
     *
     * @param connect_packet A reference to the packet.
     */
    virtual void handle_connect(const ConnectPacket & connect_packet);

    /**
     * Handle a received ConnackPacket.
     *
     * The default action is to throw an exception. Subclasses should override this method.
     *
     * @param connack_packet A reference to the packet.
     */
    virtual void handle_connack(const ConnackPacket & connack_packet);

    /**
     * Handle a received PublishPacket.
     *
     * The default action is to throw an exception. Subclasses should override this method.
     *
     * @param publish_packet A reference to the packet.
     */
    virtual void handle_publish(const PublishPacket & publish_packet);

    /**
     * Handle a received PubackPacket.
     *
     * The default action is to throw an exception. Subclasses should override this method.
     *
     * @param puback_packet A reference to the packet.
     */
    virtual void handle_puback(const PubackPacket & puback_packet);

    /**
     * Handle a received PubrecPacket.
     *
     * The default action is to throw an exception. Subclasses should override this method.
     *
     * @param pubrec_packet A reference to the packet.
     */
    virtual void handle_pubrec(const PubrecPacket & pubrec_packet);

    /**
     * Handle a received PubrelPacket.
     *
     * The default action is to throw an exception. Subclasses should override this method.
     *
     * @param pubrel_packet A reference to the packet.
     */
    virtual void handle_pubrel(const PubrelPacket & pubrel_packet);

    /**
     * Handle a received PubcompPacket.
     *
     * The default action is to throw an exception. Subclasses should override this method.
     *
     * @param pubcomp_packet A reference to the packet.
     */
    virtual void handle_pubcomp(const PubcompPacket & pubcomp_packet);

    /**
     * Handle a received SubscribePacket.
     *
     * The default action is to throw an exception. Subclasses should override this method.
     *
     * @param subscribe_packet A reference to the packet.
     */
    virtual void handle_subscribe(const SubscribePacket & subscribe_packet);

    /**
     * Handle a received SubackPacket.
     *
     * The default action is to throw an exception. Subclasses should override this method.
     *
     * @param suback_packet A reference to the packet.
     */
    virtual void handle_suback(const SubackPacket & suback_packet);

    /**
     * Handle a received UnsubscribePacket.
     *
     * The default action is to throw an exception. Subclasses should override this method.
     *
     * @param unsubscribe_packet A reference to the packet.
     */
    virtual void handle_unsubscribe(const UnsubscribePacket & unsubscribe_packet);

    /**
     * Handle a received UnsubackPacket.
     *
     * The default action is to throw an exception. Subclasses should override this method.
     *
     * @param unsuback_packet A reference to the packet.
     */
    virtual void handle_unsuback(const UnsubackPacket & unsuback_packet);

    /**
     * Handle a received PingreqPacket.
     *
     * The default action is to send a Pingresp packet. Subclasses can override this method.
     *
     * @param pingreq_packet A reference to the packet.
     */
    virtual void handle_pingreq(const PingreqPacket & pingreq_packet);

    /**
     * Handle a received PingrespPacket.
     *
     * The default action is to do nothing. Subclasses can override this method.
     *
     * @param pingresp_packet A reference to the packet.
     */
    virtual void handle_pingresp(const PingrespPacket & pingresp_packet);

    /**
     * Handle a received DisconnectPacket.
     *
     * The default action is to throw an exception. Subclasses should override this method.
     *
     * @param disconnect_packet A reference to the packet.
     */
    virtual void handle_disconnect(const DisconnectPacket & disconnect_packet);

    std::unique_ptr<PacketManager> packet_manager;

};