//
// Created by Scott Brumbaugh on 8/5/16.
//

#include "gtest/gtest.h"

#include "topic.h"

TEST(topic_filters, valid_topic_filters) {

    std::vector<std::string> valid_filters = {
            "a/b/c",
            "a",
            "",
            "/",
            "+/+",
            "//",
            "/+/+/",
            "#",
            "/#",
            "+/#"};

    for (auto filter_string : valid_filters) {
        TopicFilter f(filter_string);
    }

}

TEST(topic_filters, invalid_topic_filters) {

    std::vector<std::string> invalid_filters = {
            "a#b",
            "++/",
            "/#/",
            "#/",
            "+a",
            "a+",
            "a+b",
            "a+/"};

    for (auto filter_string : invalid_filters) {
        ASSERT_THROW(TopicFilter f(filter_string), std::exception);
    }

}

TEST(subscription_matching, matching_filter_names) {

    typedef std::string filter_t;
    typedef std::string topic_t;

    std::vector<std::pair<filter_t, topic_t>> matching_subscriptions = {
            {"a/b/c", "a/b/c"},
            {"+/b/c", "a/b/c"},
            {"a/+/c", "a/b/c"},
            {"a/b/+", "a/b/c"},
            {"a/#", "a/b/c"},
            {"#", "a/b/c"},
            {"+/b/#", "a/b/c"},
            {"+/+/+", "a/b/c"},
    };

    for (auto subscription : matching_subscriptions) {
        TopicFilter f(subscription.first);
        TopicName n(subscription.second);

        ASSERT_TRUE(topic_match(f, n));
    }

}

TEST(subscription_matching, non_matching_filter_names) {

    typedef std::string filter_t;
    typedef std::string topic_t;

    std::vector<std::pair<filter_t, topic_t>> non_matching_subscriptions = {
            {"a/b/", "a/b/c"},
            {"+/b/", "a/b/c"},
            {"a//c", "a/b/c"},
            {"a/b/+/", "a/b/c"},
            {"/#",   "a/b/c"},
            {"",     "a/b/c"},
            {"+//#", "a/b/c"},
            {"+//+", "a/b/c"},
    };

    for (auto subscription : non_matching_subscriptions) {
        TopicFilter f(subscription.first);
        TopicName n(subscription.second);

        ASSERT_FALSE(topic_match(f, n));
    }

}