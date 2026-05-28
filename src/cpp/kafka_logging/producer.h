#ifndef KAFKA_PRODUCER_H
#define KAFKA_PRODUCER_H

#include <string>
#include <librdkafka/rdkafkacpp.h>

#include "../cache_event.h"

class CacheEventsKafkaProducer {
    const std::string brokers;
    const std::string cache_events_topic; 
    
    RdKafka::Producer* producer;

    public:

    CacheEventsKafkaProducer(const std::string& brokers, const std::string& cache_events_topic);

    ~CacheEventsKafkaProducer();

    void produce_cache_event(const CacheEvent& event);
};

#endif
