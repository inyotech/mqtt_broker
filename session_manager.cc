//
// Created by Scott Brumbaugh on 8/8/16.
//

#include "session_manager.h"

#include "session.h"
#include "packet_manager.h"
#include "topic.h"

#include <memory>

void SessionManager::accept_connection(struct bufferevent *bev) {

    auto session = std::unique_ptr<Session>(new Session(bev, *this));
    sessions.push_back(std::move(session));
}

std::unique_ptr<Session> SessionManager::find_session(const std::string &client_id) {

    for (auto s = sessions.begin(); s != sessions.end(); ++s) {
         if (!(*s)->client_id.empty() and ((*s)->client_id == client_id)) {
            auto old_sess = std::move(*s);
            sessions.erase(s);
            return old_sess;
        }
    }
    return nullptr;
}

void SessionManager::handle_publish(const PublishPacket & packet) {
    for (auto &session : sessions) {
        for (auto &subscription : session->subscriptions) {
            if (topic_match(subscription.topic_filter, TopicName(packet.topic_name))) {
                    session->forward_packet(packet);
            }
        }
    }
}