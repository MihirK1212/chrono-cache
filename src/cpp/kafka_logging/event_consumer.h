#ifndef KAFKA_CACHE_EVENT_CONSUMER_H
#define KAFKA_CACHE_EVENT_CONSUMER_H

#include <string>
#include <vector>

#include "../cache_event.h"
#include "consumer.h"

class CacheEventConsumer {
    CacheEventsKafkaConsumer consumer;

public:
    CacheEventConsumer(const std::string& brokers, const std::string& topic);

    std::vector<CacheEvent> consume_all_events();
};

#endif
