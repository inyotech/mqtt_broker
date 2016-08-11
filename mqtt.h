//
// Created by Scott Brumbaugh on 8/11/16.
//

#pragma once

#include <memory>

class Session;
class PacketManager;

typedef std::unique_ptr<Session> owned_session_ptr_t;
typedef std::unique_ptr<PacketManager> owned_packet_manager_ptr_t;