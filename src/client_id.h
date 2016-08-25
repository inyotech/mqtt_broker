/**
 * @file client_id.h
 *
 * Function to generate a client id.
 *
 * This function is used to generate a random client id for clients that don't provide one in their connect control
 * packet.
 */
#pragma once

#include <string>
#include <cstdint>

/**
 * Generate a random client id.
 *
 * The client id is a random character sequence from the character set [a-z0-9].
 *
 * @param len Length of the sequence to generate
 * @return    Random character sequence
 */
std::string generate_client_id(size_t len=32);