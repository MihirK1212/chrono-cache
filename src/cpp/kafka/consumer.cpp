#include <iostream>

#include "consumer.h"

CacheEventsKafkaConsumer::CacheEventsKafkaConsumer(const std::string& brokers, const std::string& cache_events_topic) 
    : brokers(brokers), cache_events_topic(cache_events_topic) 
{
    std::string errstr; 

    RdKafka::Conf* conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    conf->set("bootstrap.servers", brokers, errstr);
    conf->set("group.id", "chrono-cache-events-consumer-group", errstr); // not really required since we are not using a consumer group, but it's good to have it
    conf->set("enable.auto.commit", "false", errstr); // not required since we always start from the beginning of the topic
    conf->set("auto.offset.reset", "earliest", errstr);
    conf->set("enable.partition.eof", "true", errstr);

    consumer = RdKafka::KafkaConsumer::create(conf, errstr);

    if (!consumer) {
        std::cerr << "Failed to create consumer: " << errstr << std::endl;
        exit(1);
    }
    
    delete conf;
}


CacheEventsKafkaConsumer::~CacheEventsKafkaConsumer() {
    consumer->close();
    delete consumer;
}

int CacheEventsKafkaConsumer::get_num_partitions() const {
    // TODO: replace later with dynamic metadata fetching
    return NUM_PARTITIONS;
}

int CacheEventsKafkaConsumer::assign_all_partitions() {
    int num_partitions = get_num_partitions();
    if (num_partitions <= 0) {
        std::cerr << "No partitions found" << std::endl;
        exit(1);
    }

    std::vector<RdKafka::TopicPartition*> partitions;
    for (int p = 0; p < num_partitions; ++p) {
        partitions.push_back(
            RdKafka::TopicPartition::create(
                cache_events_topic,
                p,
                0
            )
        );
    }

    // here, we are manually assinging all the paritions to the consumer 
    // since we want to read all the events from all the partions at the same time 
    // launching multiple consumers with this consumer group makes no sense, 
    // because all of them will read from all the partions, which is just extra work
    RdKafka::ErrorCode err = consumer->assign(partitions);

    if (err != RdKafka::ERR_NO_ERROR) {
        std::cerr << "Assign failed: "
                << RdKafka::err2str(err)
                << std::endl;
        exit(1);
    }

    return num_partitions;
}

std::vector<std::string> CacheEventsKafkaConsumer::consume_all_events() {
    std::vector<std::string> events;
    int num_partitions = assign_all_partitions(); 
    if (num_partitions <= 0) {
        std::cerr << "No partitions assigned" << std::endl;
        exit(1);
    }

    std::cout << "All " << num_partitions << " partitions assigned, starting to consume events" << std::endl;

    int count_reached_eof = 0;

    while (count_reached_eof < num_partitions) {
        RdKafka::Message* msg = consumer->consume(5000);

        switch (msg->err()) {
            case RdKafka::ERR_NO_ERROR: {
                std::string payload(
                    static_cast<const char*>(msg->payload()),
                    msg->len()
                );
                std::cout << " Received event from [Partition "
                          << msg->partition()
                          << " | Offset "
                          << msg->offset()
                          << "] "
                          << payload
                          << std::endl;
                events.push_back(payload);
                break;
            }

            case RdKafka::ERR__PARTITION_EOF:
                count_reached_eof++;
                std::cout << "Reached end of partition " << count_reached_eof << std::endl;
                break;

            case RdKafka::ERR__TIMED_OUT:
                std::cout << "Timeout while consuming events" << std::endl;
                break;

            default:
                std::cerr << "Consumer error: "
                          << msg->errstr()
                          << std::endl;
        }

        delete msg;
    }

    std::cout << "Consumed all events from all partitions" << std::endl;

    return events;
}