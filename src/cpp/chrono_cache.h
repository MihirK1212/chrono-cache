#ifndef CHRONO_CACHE_H
#define CHRONO_CACHE_H

#include <chrono>
#include <optional>
#include <string>

#include "cache_entry.h"
#include "kv_store/hash_map.h"
#include "sorted_sets/sorted_set.h"
#include "kafka_logging/event_logger.h"
#include "kafka_logging/event_consumer.h"
#include "cache_config.h"

static constexpr int CHRONO_CACHE_INITIAL_CAPACITY = 16;

class ChronoCache {
    ChronCacheHashMap<std::string, CacheEntry> kv_store;
    SortedSetsAPI sorted_sets;

    CacheEventLogger cache_event_logger;
    CacheEventConsumer cache_event_consumer;

    public:
    
    ChronoCache(const CacheConfig& config);

    bool set(const std::string& key, const std::string& value,
             std::optional<std::chrono::milliseconds> ttl = std::nullopt);

    std::optional<std::string> get(const std::string& key);

    bool del(const std::string& key);

    bool expire(const std::string& key, std::chrono::milliseconds ttl);

    bool persist(const std::string& key);

    long long pttl(const std::string& key);

    bool zadd(const std::string& key, double score, const std::string& member);
    std::optional<double> zscore(const std::string& key, const std::string& member) const;
    bool zrem(const std::string& key, const std::string& member);
    std::optional<int> zrank(const std::string& key, const std::string& member) const;
};

#endif
