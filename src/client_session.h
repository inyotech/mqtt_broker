//
// Created by Scott Brumbaugh on 8/23/16.
//

#pragma once

#include "session_base.h"

class ClientSession : public SessionBase
{

    void handle_connack(const ConnackPacket &) override;

    void handle_suback(const SubackPacket &) override;

    void handle_publish(const PublishPacket &) override;

    void handle_pubrel(const PubrelPacket &) override;

    void handle_pingresp(const PingrespPacket &) override;

};