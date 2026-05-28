#ifndef CHRONO_CACHE_H
#define CHRONO_CACHE_H

#include <chrono>
#include <optional>
#include <string>

#include "cache_entry.h"
#include "kv_store/hash_map.h"
#include "sorted_sets/sorted_set.h"
#include "kafka_logging/event_logger.h"
#include "cache_config.h"
#include "cache_state.h"

static constexpr int CHRONO_CACHE_INITIAL_CAPACITY = 16;

class ChronoCache {
    ChronoCacheState state;

    ChronCacheHashMap<std::string, CacheEntry> hash_map;
    SortedSetsAPI sorted_sets;

    bool disable_event_logging;
    std::string kafka_brokers;
    std::string kafka_topic;
    std::optional<CacheEventLogger> cache_event_logger;

    bool is_logging_allowed() const;
    CacheEventLogger& ensure_logger();
    void check_accepting_ops();
    void replay();

    public:

    ChronoCache(const CacheConfig& config);

    bool is_accepting_ops() const;

    bool init(bool with_replay);

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
