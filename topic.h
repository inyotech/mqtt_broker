//
// Created by Scott Brumbaugh on 8/5/16.
//

#pragma once

#include <string>

class TopicFilter;

class TopicName {
public:
    const static size_t MaxNameSize = 65535;

    TopicName(const std::string &);
    bool is_valid(const std::string &) const;
    operator std::string() const {return name;}

    friend bool topic_match(const TopicFilter &, const TopicName &);

private:
    std::string name;
};

class TopicFilter {
public:
    const static size_t MaxFilterSize = 65535;

    TopicFilter(const std::string &);
    bool is_valid(const std::string &) const;
    operator std::string() const {return filter;}

    friend bool topic_match(const TopicFilter &, const TopicName &);
    friend bool topic_match(const TopicFilter &, const TopicFilter &);

private:
    std::string filter;
};

bool topic_match(const TopicFilter &, const TopicName &);

bool topic_match(const TopicFilter &, const TopicFilter &);
