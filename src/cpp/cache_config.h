#ifndef CACHE_CONFIG_H
#define CACHE_CONFIG_H

#include <string>

struct CacheConfig {
    std::string kafka_brokers;
    std::string kafka_cache_events_topic;
    bool disable_event_logging;

    CacheConfig(const std::string& kafka_brokers, const std::string& kafka_cache_events_topic, bool disable_event_logging = false)
        : kafka_brokers(kafka_brokers)
        , kafka_cache_events_topic(kafka_cache_events_topic)
        , disable_event_logging(disable_event_logging) {}
};

#endif