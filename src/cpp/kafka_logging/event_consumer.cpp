#include "event_consumer.h"

CacheEventConsumer::CacheEventConsumer(const std::string& brokers, const std::string& topic)
    : consumer(brokers, topic) {}

std::vector<CacheEvent> CacheEventConsumer::consume_all_events() {
    return consumer.consume_all_events();
}
