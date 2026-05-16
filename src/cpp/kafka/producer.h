#ifndef KAFKA_PRODUCER_H
#define KAFKA_PRODUCER_H

#include <string>
#include <librdkafka/rdkafkacpp.h>

class CacheEventsKafkaProducer {
    const std::string brokers;
    const std::string cache_events_topic; 
    
    RdKafka::Producer* producer;

    public:

    CacheEventsKafkaProducer(const std::string& brokers, const std::string& cache_events_topic);

    ~CacheEventsKafkaProducer();

    void produce_random_message(std::string key, std::string value);
};

#endif