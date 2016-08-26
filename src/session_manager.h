/**
 * @file session_manager.h
 */

#pragma once

#include <list>
#include <string>
#include <memory>

struct bufferevent;

class BrokerSession;
class PublishPacket;

class SessionManager
{
public:

    void accept_connection(struct bufferevent * bev);

    std::list<std::unique_ptr<BrokerSession>>::iterator find_session(const std::string & client_id);

    void remove_session(const BrokerSession *);
    void remove_session(const std::string & client_id);

    void handle_publish(const PublishPacket &);

    std::list<std::unique_ptr<BrokerSession>> sessions;

};