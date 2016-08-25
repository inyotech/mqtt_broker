//
// Created by Scott Brumbaugh on 8/4/16.
//

#include "client_id.h"

#include <cstdlib>
#include <ctime>
#include <mutex>

static const std::string characters = "abcdefghijklmnopqrstuvwxyz0123456789";
static std::once_flag init_rnd;

std::string generate_client_id(size_t len) {

    std::call_once(init_rnd, [](){ std::srand(std::time(nullptr)); });

    std::string random_string;

    for (size_t i=0; i<len; i++) {
        random_string += characters[std::rand() % characters.size()];
    }

    return random_string;

}