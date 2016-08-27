/**
 * @file session_manager.h
 *
 * Manage BrokerSessions.
 *
 * The SessionManager maintains a contaier of all sessions in a broker.  BrokerConnects are created in the session
 * manager when a network session is accepted the new session is added to the sessions container.  The session is then
 * responsible for managing the MQTT protocol.  The MQTT 3.1.1 standard requires that sessions
 * can persist after a client disconnects and that any subscribe QoS 1 and Qo2 messages published while disconnected be
 * delivered on reconnection.
 *
 * The SessionManager is responsible for forwarding published messages to all subscribing clients.
 */

#pragma once

#include <list>
#include <string>
#include <memory>

struct bufferevent;

class BrokerSession;
class PublishPacket;

/**
 * SessionManager class
 *
 * Composes a container of broker sessions and methods to manage them.
 */
class SessionManager
{
public:

    /**
     * Accept a new network connection.
     *
     * Creates a new BrokerSession instance and adds it to the container of sessions.  The session instance will manage
     * the MQTT protocol.
     *
     * @param bev Pointer to a bufferevent
     */
    void accept_connection(struct bufferevent * bev);

    /**
     * Find a session in the session container.
     *
     * @param client_id Unique client id to find.
     * @return          Iterator to BrokerSession.
     */
    std::list<std::unique_ptr<BrokerSession>>::iterator find_session(const std::string & client_id);

    /**
     * Delete a session
     *
     * Given a pointer to a BrokerSession, finds that session in the session container and removes it from the
     * container.  The session instance will be deleted.
     *
     * @param session Pointer to a BrokerSession;
     */
    void erase_session(const BrokerSession *session);

    /**
     * Finds a session in the session container with the given client id.  If found the session is removed from the
     * container.  The session instance will be deleted.
     *
     * @param client_id A Client id.
     */
    void erase_session(const std::string &client_id);

    /**
     * Forward a message to subsribed clients.
     *
     * Searches through each session and their subscriptions and invokes the forward_packet method on each session
     * instance with a matching subscribed TopicFilter.  The session will be responsible for Managing the MQTT publish
     * protocol and correctly delivering the message to its subscribed client
     *
     * @param publish_packet Reference to a PublishPacket;
     */
    void handle_publish(const PublishPacket & publish_packet);

    /** Container of BrokerSessions. */
    std::list<std::unique_ptr<BrokerSession>> sessions;

};