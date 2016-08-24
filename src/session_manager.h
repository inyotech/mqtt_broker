//
// Created by Scott Brumbaugh on 8/8/16.
//

#pragma once

#include <list>
#include <memory>

#include <event2/bufferevent.h>

class Session;
class PublishPacket;

class SessionManager
{
public:

    void accept_connection(struct bufferevent * bev);

    std::list<std::unique_ptr<Session>>::iterator find_session(const std::string & client_id);

    void remove_session(const Session *);
    void remove_session(const std::string & client_id);

    void handle_publish(const PublishPacket &);

    std::list<std::unique_ptr<Session>> sessions;

};