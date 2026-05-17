#ifndef KAFKA_EVENT_CONSUMER_H
#define KAFKA_EVENT_CONSUMER_H

#include <string>
#include <vector>

#include "../cache_event.h"
#include "consumer.h"

class EventConsumer {
    CacheEventsKafkaConsumer consumer;

public:
    EventConsumer(const std::string& brokers, const std::string& topic);

    std::vector<CacheEvent> consume_all_events();
};

#endif
