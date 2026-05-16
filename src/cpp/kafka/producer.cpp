#include <iostream>

#include <nlohmann/json.hpp>

#include "producer.h"

CacheEventsKafkaProducer::CacheEventsKafkaProducer(const std::string& brokers, const std::string& cache_events_topic) 
    : brokers(brokers), cache_events_topic(cache_events_topic) 
{
    std::string errstr; 

    RdKafka::Conf* conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    conf->set("bootstrap.servers", brokers, errstr);

    producer = RdKafka::Producer::create(conf, errstr);

    if (!producer) {
        std::cerr << "Failed to create producer: " << errstr << std::endl;
        exit(1);
    }

    delete conf;
}


CacheEventsKafkaProducer::~CacheEventsKafkaProducer() {
    producer->flush(10000);
    delete producer;
}

void CacheEventsKafkaProducer::produce_cache_event(const CacheEvent& event) {
    nlohmann::json j;
    j["type"]      = event_type_name(event.type);
    j["key"]       = event.key;
    j["value"]     = event.value;
    j["ttl_ms"]    = event.ttl_ms;
    j["seq"]       = event.seq;
    j["timestamp"] = event.timestamp;

    std::string payload = j.dump();

    RdKafka::ErrorCode resp = producer->produce(
        cache_events_topic,
        RdKafka::Topic::PARTITION_UA,
        RdKafka::Producer::RK_MSG_COPY,
        const_cast<char*>(payload.c_str()),
        payload.size(),
        event.key.c_str(),
        event.key.size(),
        0,
        nullptr
    );

    if (resp != RdKafka::ERR_NO_ERROR) {
        std::cerr << "Produce failed: "
                  << RdKafka::err2str(resp)
                  << std::endl;
    } else {
        std::cout << "Produced CacheEvent: type=" << event_type_name(event.type)
                  << " key=" << event.key
                  << " value=" << event.value
                  << std::endl;
    }
}
