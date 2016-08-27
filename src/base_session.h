/**
 * @file base_session.h
 *
 * Base class for MQTT sessions.  This class add facilities for persistence and resumption of session state. The MQTT
 * standard requires that the client and server both maintain session state while connected.  The server is also
 * required to resume the state when a client re-connects with the same client id.
 */

#pragma once

#include "packet.h"
#include "packet_manager.h"

#include <event2/bufferevent.h>

#include <string>

/**
 * Base session class
 *
 * Maintains session attributes and provides default handler methods for received control packets.  Classes derived
 * from BaseSession will override control packet handlers as required.
 *
 * Each BaseSession composes a PacketManager instance that can be moved between BaseSession instances.
 */
class BaseSession {

public:

    /**
     * Constructor
     *
     * Accepts a pointer to a libevent bufferevent as the only argument.  The bufferevent is forwarded to a
     * newly instantiated PacketManager that use it to handle all network related functions.
     *
     * This BaseSession instance can persist in memory after the network connection is closed.  If a connection is
     * received and the Connect control packet contains the same client id as an existing session.  Any currently
     * active connection in the original session is closed and this PacketManager will be moved to the original
     * session.  This session will then be deleted.
     *
     * @param bev Pointer to a bufferevent.
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

    /** Client id. */
    std::string client_id;

    /** Clean session flag. */
    bool clean_session;

    /**
     * PacketManager callback.
     *
     * Invoked by the installed PacketManager instance when it receives a complete control packet.  The default handler
     * methods will be passed a reference to the received packet.  Packet memory is heap allocated on creation and
     * will be freed according to standard C++ std::unique_ptr rules.  It is the responsibility of subclasses to
     * manage the std::unique_ptr<Packet>.
     *
     * @param packet Pointer to a packet.
     */
    virtual void packet_received(std::unique_ptr<Packet> packet);

    /**
     * PacketManager callback.
     *
     * Invoked by the installed PacketManager instance when it detects a low level protocol or network error.  The
     * default action is to close the network connection.
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

    /** Pointer to the installed PacketManager instance. */
    std::unique_ptr<PacketManager> packet_manager;

};