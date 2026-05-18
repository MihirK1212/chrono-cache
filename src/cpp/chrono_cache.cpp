#include "chrono_cache.h"

ChronoCache::ChronoCache(const CacheConfig& config)
    : kv_store(CHRONO_CACHE_INITIAL_CAPACITY)
    , sorted_sets()
    , disable_event_logging(config.disable_event_logging)
    , cache_event_logger(config.disable_event_logging
        ? std::nullopt
        : std::make_optional<CacheEventLogger>(config.kafka_brokers, config.kafka_cache_events_topic))
    , cache_event_consumer(config.disable_event_logging
        ? std::nullopt
        : std::make_optional<CacheEventConsumer>(config.kafka_brokers, config.kafka_cache_events_topic)) {}

bool ChronoCache::set(const std::string& key, const std::string& value, std::optional<std::chrono::milliseconds> ttl) 
{
    CacheEntry entry = ttl.has_value()
        ? CacheEntry(value, *ttl)
        : CacheEntry(value);

    return kv_store.set(key, entry, [&]() {
        if (!disable_event_logging && cache_event_logger) {
            cache_event_logger->log_set(key, value, ttl.has_value() ? ttl->count() : 0);
        }
    });
}

std::optional<std::string> ChronoCache::get(const std::string& key) 
{
    std::optional<std::string> result;
    kv_store.process_and_remove_if(key, [&](CacheEntry* e) {
        if (!e) return false;
        if (e->is_expired()) return true;
        result = e->get_value();
        return false;
    });
    return result;
}

bool ChronoCache::del(const std::string& key) 
{
    return kv_store.remove(key, [&]() {
        if (!disable_event_logging && cache_event_logger) {
            cache_event_logger->log_del(key);
        }
    });
}

bool ChronoCache::expire(const std::string& key, std::chrono::milliseconds ttl) {
    bool found = false;
    std::string current_value;
    kv_store.process_and_remove_if(key, [&](CacheEntry* e) {
        if (!e) return false;
        if (e->is_expired()) return true;
        e->update_ttl(ttl);
        current_value = e->get_value();
        found = true;
        if (!disable_event_logging && cache_event_logger) {
            cache_event_logger->log_expire(key, current_value, ttl.count());
        }
        return false;
    });

    return found;
}

long long ChronoCache::pttl(const std::string& key) {
    long long result = -2;
    kv_store.process_and_remove_if(key, [&](CacheEntry* e) {
        if (!e) return false;
        if (e->is_expired()) return true;
        result = e->has_ttl() ? e->remaining_ttl()->count() : -1;
        return false;
    });
    return result;
}

bool ChronoCache::persist(const std::string& key) {
    bool found = false;
    std::string current_value;
    kv_store.process_and_remove_if(key, [&](CacheEntry* e) {
        if (!e) return false;
        if (e->is_expired()) return true;
        found = true; 
        if (e->has_ttl()) {
            e->remove_ttl();
            current_value = e->get_value();
            if (!disable_event_logging && cache_event_logger) {
                cache_event_logger->log_persist(key, current_value);
            }
        }
        return false;
    });

    return found;
}

bool ChronoCache::zadd(const std::string& key, double score, const std::string& member) {
    return sorted_sets.zadd(key, score, member);
}

std::optional<double> ChronoCache::zscore(const std::string& key, const std::string& member) const {
    return sorted_sets.zscore(key, member);
}

bool ChronoCache::zrem(const std::string& key, const std::string& member) {
    return sorted_sets.zrem(key, member);
}

std::optional<int> ChronoCache::zrank(const std::string& key, const std::string& member) const {
    return sorted_sets.zrank(key, member);
}
