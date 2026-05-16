#ifndef KAFKA_CONSUMER_H
#define KAFKA_CONSUMER_H

#include <string>
#include <librdkafka/rdkafkacpp.h>

class CacheEventsKafkaConsumer {
    const int NUM_PARTITIONS = 4;

    const std::string brokers;
    const std::string cache_events_topic; 
    
    RdKafka::KafkaConsumer* consumer;

    int get_num_partitions() const;
    int assign_all_partitions();

    public:

    CacheEventsKafkaConsumer(const std::string& brokers, const std::string& cache_events_topic);

    ~CacheEventsKafkaConsumer();

    std::vector<std::string> consume_all_events();
};




#endif