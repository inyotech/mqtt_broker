//
// Created by Scott Brumbaugh on 8/5/16.
//

#include "topic.h"

#include <regex>

TopicName::TopicName(const std::string &s) {
    if (!is_valid(s)) {
        throw std::exception();
    }

    name = s;
}

bool TopicName::is_valid(const std::string &s) const {
    if (s.size() > MaxNameSize) {
        return false;
    }

    if ((s.find('+') == std::string::npos) and (s.find('#') == std::string::npos)) {
        return true;
    }

    return false;
}

TopicFilter::TopicFilter(const std::string &s) {
    if (!is_valid(s)) {
        throw std::exception();
    }


    filter = s;
}

bool TopicFilter::is_valid(const std::string &s) const {

    if (s.size() > MaxFilterSize) {
        return false;
    }

    size_t pos = 0;

    for (char c : s) {
        if (c == '+') {
            if ((pos != 0 and s[pos - 1] != '/') or (pos + 1 != s.size() and s[pos + 1] != '/')) {
                return false;
            }
        } else if (c == '#') {
            if ((pos != 0 and s[pos - 1] != '/') or (pos + 1 != s.size())) {
                return false;
            }
        }
        pos++;
    }

    return true;
}

bool topic_match(const TopicFilter &filter, const TopicName &name) {

    const std::string &f = filter.filter;
    const std::string &n = name.name;

    // empty strings don't match
    if (f.empty() or n.empty()) {
        return false;
    }

    size_t fpos = 0;
    size_t npos = 0;

    // Cannot match $ with wildcard
    if ((f[fpos] == '$' and n[npos] != '$') or (f[fpos] != '$' and n[npos] == '$')) {
        return false;
    }

    while (fpos < f.size() and npos < n.size()) {

        // filter and topic name match at the current position
        if (f[fpos] == n[npos]) {

            // last character in the topic name
            if (npos == n.size() - 1) {

                // at the end of the topic name and the filter has a separator followed by a multi-level wildcard,
                // causing a parent level match.
                if ((fpos == f.size() - 3) and (f[fpos + 1] == '/') and (f[fpos + 2] == '#')) {
                    return true;
                }
            }

            fpos++;
            npos++;

            // at the end of both the filter and topic name, match
            if ((fpos == f.size()) && (npos == n.size())) {
                return true;

            // at the end of the topic name and the next character in the filter is wildcard.
            } else if ((npos == n.size()) and (fpos == f.size() - 1) and (f[fpos] == '+')) {
                return true;
            }

        } else {

            if (f[fpos] == '+') {
                fpos++;
                while ((npos < n.size()) and (n[npos] != '/')) {
                    npos++;
                }
                if ((npos == n.size()) and (fpos == f.size())) {
                    return true;
                }
            } else if (f[fpos] == '#') {
                return true;
            } else {
                return false;
            }
        }
    }

    return false;
}

bool topic_match(const TopicFilter &filter1, const TopicFilter &filter2) {

   return filter1.filter == filter2.filter;

}