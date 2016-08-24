//
// Created by Scott Brumbaugh on 8/4/16.
//

#include "client_id.h"
#include <ossp/uuid.h>

#include <cstdlib>

std::string generate_client_id() {

    uuid_t * uuid;
    char * c_str = NULL;

    uuid_create(&uuid);
    uuid_make(uuid, UUID_MAKE_V4);
    uuid_export(uuid, UUID_FMT_STR, &c_str, NULL);
    uuid_destroy(uuid);

    std::string uuid_str(c_str);

    free(c_str);

    return std::string(uuid_str);

}