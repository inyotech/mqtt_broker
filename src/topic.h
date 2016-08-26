/**
 * @file topic.h
 *
 * Classes for managing topic names and topic filters.
 *
 * The MQTT 3.1.1 standard allows structured topic names. It also defines rules for matching these names and provides
 * wildcard characters to enhance matching rules.
 */

#pragma once

#include <string>

class TopicFilter;

/**
 * Topic Name
 *
 * Topic names are UTF-8 encoded character strings.  The have a structure imposed by the MQTT 3.1.1 standard.  This
 * class enforces that structure.  Topic names differe from topic filters in that topic filters allow wildcard
 * characters.
 *
 * This class friends the topic_match function.
 */
class TopicName {
public:

    /** Maximum lenght of a topic name according the the MQTT 3.1.1 standard. */
    const static size_t MaxNameSize = 65535;

    /**
     * Constructor
     *
     * The string will be validated against the MQTT topic name rules.  An exception will be thrown if the name is
     * invalid.
     *
     * @param name A reference to a the topic name string.
     */
    TopicName(const std::string & name);

    /**
     * Validate the topic name against the MQTT 3.1.1 standard rules.
     *
     * @param name A name string.
     * @return     Topic name is valid.
     */
    bool is_valid(const std::string & name) const;

    /**
     * Cast an instance of this class to a std::string.
     *
     * @return std::string
     */
    operator std::string() const {return name;}

    /** Matching friend function. */
    friend bool topic_match(const TopicFilter &, const TopicName &);

private:

    /** The name character string. */
    std::string name;
};

/**
 * Topic Filter
 *
 * Topic filters are composed of UTF-8 encoded character strings.  The have a structure imposed by the MQTT 3.1.1
 * standard including wildcard characters.  This class enforces that structure.
 *
 * This class friends the topic_match function.
 */
class TopicFilter {
public:

    /** Maximum lenght of a topic filter according the the MQTT 3.1.1 standard. */
    const static size_t MaxFilterSize = 65535;

    /**
     * Constructor
     *
     * The string will be validated against the MQTT topic filter rules.  An exception will be thrown if the filter is
     * invalid.
     *
     * @param filter A reference to a the topic filter string.
     */
    TopicFilter(const std::string & filter);

    /**
     * Validate the topic filter against the MQTT 3.1.1 standard rules.
     *
     * @param filter A filter string.
     * @return     Topic filter is valid.
     */
    bool is_valid(const std::string & filter) const;

    /**
     * Cast an instance of this class to a std::string.
     *
     * @return std::string
     */
    operator std::string() const {return filter;}

    /** Matching friend function. */
    friend bool topic_match(const TopicFilter &, const TopicName &);

    /** Matching friend function. */
    friend bool topic_match(const TopicFilter &, const TopicFilter &);

private:

    /** The filter character string. */
    std::string filter;
};

/**
 * Match a TopicFilter against a TopicName.
 *
 * The MQTT 3.1.1 standard topic filter matching rules will be applied including wildcard characters.
 *
 * @param topic_filter A reference to a TopicFilter.
 * @param topic_name   A reference to a TopicName
 * @return match
 */
bool topic_match(const TopicFilter & topic_filter, const TopicName & topic_name);

/**
 * Match a TopicFilter against another TopicFilter.
 *
 * The MQTT 3.1.1 standard topic filter matching rules are applied.  These are a direct character by character match.
 * This function can be used when finding an existing subscription filter, in that case wildcard character matching
 * does not apply.
 *
 * For example, the topic filters 'a/+/c' and 'a/#' both match the topic name 'a/b/c' but the two topic filters do not
 * match.
 *
 * @param topic_filter A reference to a TopicFilter.
 * @param topic_name   A reference to a TopicName
 * @return match
 */
bool topic_match(const TopicFilter & topic_filter, const TopicFilter & topic_name);
