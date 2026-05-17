#ifndef KAFKA_EVENT_LOGGER_H
#define KAFKA_EVENT_LOGGER_H

#include <cstdint>
#include <string>
#include <unordered_map>

#include "producer.h"

class EventLogger {
    CacheEventsKafkaProducer producer;
    std::unordered_map<std::string, uint64_t> seq_counters;

    uint64_t next_seq(const std::string& key);
    static int64_t now_ms();

public:
    EventLogger(const std::string& brokers, const std::string& topic);

    // set seq counters after replay
    void set_seq_counters(std::unordered_map<std::string, uint64_t> last_applied_seq);

    void log_set(const std::string& key, const std::string& value, int64_t ttl_ms);
    void log_del(const std::string& key);
    void log_expire(const std::string& key);
};

#endif
