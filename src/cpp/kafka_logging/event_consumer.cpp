#include "event_consumer.h"

EventConsumer::EventConsumer(const std::string& brokers, const std::string& topic)
    : consumer(brokers, topic) {}

std::vector<CacheEvent> EventConsumer::consume_all_events() {
    return consumer.consume_all_events();
}
