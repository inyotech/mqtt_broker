//
// Created by Scott Brumbaugh on 8/11/16.
//

#pragma once

#include <memory>
#include <cstdint>
#include <vector>

class Session;
class PacketManager;
class Packet;

typedef std::unique_ptr<Session> owned_session_ptr_t;

typedef std::unique_ptr<PacketManager> owned_packet_manager_ptr_t;

typedef std::unique_ptr<Packet> owned_packet_ptr_t;

typedef std::vector<uint8_t> packet_data_t;