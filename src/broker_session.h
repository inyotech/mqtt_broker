/**
 * @file broker_session.h
 *
 * This class builds on the BaseSession class adding members and methods needed for a MQTT session in the broker.
 *
 * Drived class for MQTT broker sessions.  In addition to maintaining session state throughout the lifetime of the
 * connection, the MQTT specification requires that the broker persist session state after a client closes the
 * connection.  When a subsequent connection is made with the same client id, the persisted session should be resumed.
 * Any QoS 1 or QoS 2 messages delivered to client subscribed topics should be forwarded over the new connection.
 */

#pragma once

#include "base_session.h"
#include "packet_manager.h"
#include "packet.h"

#include <event2/bufferevent.h>

#include <list>
#include <memory>

class SessionManager;

class Message;

class Subscription;

/**
 * Broker session class
 *
 * In addition to maintaining session attributes and handling control packets, this subclass adds facilities for
 * persisting and resuming session state according to the MQTT 3.1.1 standard.
 */
class BrokerSession : public BaseSession {

public:

    /**
     * Constructor
     *
     * In addition to the bufferen event pointer required by the BaseSession constructor, this constructor accepts
     * a reference to a SessionManager class.
     *
     * @param bev               Pointer to a bufferevent internal control structure.
     * @param session_manager   Reference to the SessionManager.
     */
    BrokerSession(struct bufferevent *bev, SessionManager &session_manager) : BaseSession(bev),
                                                                              session_manager(session_manager) {
    }

    /**
     * Handle authentication and authorization of a connecting client.
     *
     * This method is called from the connect packet handler.  It currently always returns true.
     *
     * @return Authorization granted.
     */
    bool authorize_connection(const ConnectPacket &);

    /**
     * Resume a persisted session.
     *
     * The MQTT 3.1.1 standard requires that the session state be restored for clients connecting with the same client
     * id.  This method is used to perform that action once a persisted session is recognized.  This method accepts
     * a reference to the BrokerSession to be restored and PacketManager instance to be installed in the restored
     * session.  Once installed a the PacketManager will send a Connack packet to the connecting client with the
     * Session Present flag set.
     *
     * @param session        Reference to the session to be resumed.
     * @param packet_manager PacketManager to be installed in the resumed session.
     */
    void resume_session(std::unique_ptr<BrokerSession> &session,
                        std::unique_ptr<PacketManager> packet_manager);

    /**
     * List of topics subscribed to by this client.
     */
    std::vector<Subscription> subscriptions;

    /**
     * Forward a publshed message to the connected client.
     *
     * This method is called by the SessionManager when forwarding messages to subscribed clients.  It will behave
     * according to the QoS in the PublishPacket.  QoS 0 packets will be forwarded and forgotten.  In the case of QoS 1
     * or 2 messages, these will be retained until they are acknowledged according to the publish control packet
     * protocol flow described in the MQTT 3.1.1 standard.
     *
     * @param packet Reference to the PublishPacket to forward.
     */
    void forward_packet(const PublishPacket &packet);

    /**
     * Send messages from the pending queues.
     *
     * Iterate through the pending message queues and if non-empty send a single pending message.  This method should
     * be called periodically.  Currently it is called each time a packet is received from a client.
     */
    void send_pending_message(void);

    /**
     * PacketManager callback.
     *
     * This method will delegate to the BaseSession method and then invoke send_pending_message.  Ownership of the
     * Packet is transfered back to the BaseSession instance.
     *
     * @param packet Reference counted pointer to a packet.
     */
    void packet_received(std::unique_ptr<Packet> packet) override;

    /**
     * PacketManager callback.
     *
     * This method will delegate to the BaseSession method then potentially remove this session from the SessionManager
     * based on the clean_session flag.
     *
     * @param event The type of event detected.
     */
    void packet_manager_event(PacketManager::EventType event) override;

    /**
     * Handle a received ConnectPacket.
     *
     * This method will examine the ConnectPacket and restore or set up a new session. The authorize_connection method
     * will be called to authenticate and authorize this connection.   The client id will be used to lookup any
     * previous session and if found the PacketManager instance installed in this session will be moved to the
     * persisted session and this session instance will be destroyed.
     *
     * In case a persisted session is not found.  A Connack packet will be sent with the session_present flag set to
     * false.
     *
     * @param connect_packet A reference to the packet.
     */
    void handle_connect(const ConnectPacket & connect_packet) override;

    /**
     * Handle a received PublishPacket.
     *
     * Delegate forwarding of this message to the SessionManager.  Additional actions will be performed based on the
     * QoS value in the PublishPacket.  For QoS 1 a Puback packet will be sent.  For QoS 2, queue and expected Pubrel
     * packet id which will initiate the sending of a Pubrec packet at the next run of the pending packets queue.
     *
     * @param publish_packet A reference to the packet.
     */
    void handle_publish(const PublishPacket & publish_packet) override;

    /**
     * Handle a received PubackPacket.
     *
     * This packet is expected in response to a Publish control packet with QoS 1.  Publish packets with QoS 1 will be
     * resent periodically until a Puback is received.  QoS 1 Publish packets have an 'at least once' delivery
     * guarantee. This handler will remove the Publish packet from the queue of pending messages ending the QoS 1
     * protocol flow.
     *
     * @param puback_packet A reference to the packet.
     */
    void handle_puback(const PubackPacket & puback_packet) override;

    /**
     * Handle a received PubrecPacket
     *
     * This packet is received in response to a Publish control packet with QoS 2.  Publish packets with QoS 2 will be
     * resent periodically until a PubRec is received.  QoS 2 Publish packets have an 'exactly once' delivery
     * guarantee.  This handler will remove the Publish packet from the queue of pending messages and will continue
     * the QoS 2 protocol flow by adding the packet id to the pending Pubcomp queue.  This will enable the send of a
     * Pubrel control packet at the next pending packet queue run.
     *
     * @param pubrec_packet A reference to the packet.
     */
    void handle_pubrec(const PubrecPacket & pubrec_packet) override;

    /**
     * Handle a received PubrelPacket.
     *
     * This packet is received in response to a Pubrec packet in the QoS 2 protocol flow.  This handler will remove
     * the Pubrec packet id from the queue of pending Pubrel packets and send a Pubcomp packet ending the QoS 2
     * protocol flow.
     *
     * @param pubrel_packet A reference to the packet.
     */
    void handle_pubrel(const PubrelPacket & pubrel_packet) override;

    /**
     * Handle a received PubcompPacket.
     *
     * This packet is received in response to a Pubrel control packet in the QoS 2 protocol flow.  This handler will
     * remove the packet id from the pending Pubcomp queue.  No further processing is done.
     *
     * @param pubcomp_packet A reference to the packet.
     */
    void handle_pubcomp(const PubcompPacket & pubcomp_packet) override;

    /**
     * Handle a received SubscribePacket.
     *
     * Add the contained topic names to the list of subscriptions maintained in this session.  Any previous matching
     * subscribed topic will be replaced by the new one overriding the subscribed QoS.  Send a Suback packet in
     * response.
     *
     * @param subscribe_packet A reference to the packet.
     */
    void handle_subscribe(const SubscribePacket & subscribe_packet) override;

    /**
     * Handle a received UnsubscribePacket.
     *
     * Remove the topic name from the list of subscribed topics.
     *
     * //TODO This function is currently incomplete.
     *
     * @param unsubscribe_packet A reference to the packet.
     */
    void handle_unsubscribe(const UnsubscribePacket & unsubscribe_packet) override;

    /**
     * Handle a DisconnectPacket.
     *
     * Remove this session from the pool of persistent sessions and destroy it, depending on the clean_session
     * attribute of this session.
     *
     * @param disconnect_packet A reference to the packet.
     */
    void handle_disconnect(const DisconnectPacket & disconnect_packet) override;

    /**
    * List of QoS 1 messages waiting for Puback.
    *
    * These messages have been forwarded to subscribed clients.  This list will be persisted between connections as
    * part of the BrokerSession state.
    */
    std::vector<PublishPacket> qos1_pending_puback;

    /**
     * List of QoS 2 messages waiting for Pubrec.
     *
     * These messages have been forwarded to subscribed clients.  This list will be persisted between connections as
     * part of the BrokerSession state.
     */
    std::vector<PublishPacket> qos2_pending_pubrec;

    /**
     * List of QoS 2 messages waiting for Pubrel.
     *
     * These messages have been received in Publish control packets from a client connected to this session and
     * potentially forwarded on by the SessionManager to other subscribed clients.  Packet ids are added to this
     * list when a Pubrec control packet has been sent.  The list will be persisted between connections as part of the
     * BrokerSession state.
     */
    std::vector<uint16_t> qos2_pending_pubrel;

    /**
     * List of QoS 2 messages waiting for Pubcomp.
     *
     * These messages have been forwarded to subscribed clients.  Packet ids are added to this list when a Pubrel
     * control packet has been received. This list will be persisted between connections as part of the BrokerSession
     * state.
     */
    std::vector<uint16_t> qos2_pending_pubcomp;

    /**
     * A reference to the SessionManager instance.
     */
    SessionManager &session_manager;

};

