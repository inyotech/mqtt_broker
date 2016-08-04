//
// Created by Scott Brumbaugh on 8/4/16.
//

#include "client_id.h"
#include <uuid/uuid.h>

std::string generate_client_id() {

    uuid_t uuid;
    uuid_generate(uuid);
    uuid_string_t uuid_str;
    uuid_unparse_lower(uuid, uuid_str);

    return std::string(uuid_str);

}