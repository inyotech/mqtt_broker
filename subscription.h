//
// Created by Scott Brumbaugh on 8/8/16.
//

#pragma once

#include "topic.h"

#include <cstdint>

class Subscription {
public:
    TopicFilter topic_filter;
    uint8_t qos;
};