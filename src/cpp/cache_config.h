#ifndef CACHE_CONFIG_H
#define CACHE_CONFIG_H

#include <string>

struct CacheConfig {
    std::string kafka_brokers;
    std::string kafka_cache_events_topic;

    CacheConfig(const std::string& kafka_brokers, const std::string& kafka_cache_events_topic)
        : kafka_brokers(kafka_brokers)
        , kafka_cache_events_topic(kafka_cache_events_topic) {}
};

#endif