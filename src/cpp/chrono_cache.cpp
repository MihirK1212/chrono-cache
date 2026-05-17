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

// get, expire, pttl, and persist all follow the same pattern:
// acquire the write lock, then operate via _unguarded methods under that lock.
//
// Why not use the public map API (get, remove) directly?
// get_ptr_unguarded returns a raw pointer into the map's internal node. That pointer
// is only valid while the stripe lock is held — a concurrent remove() on the same key
// would delete the node and leave the pointer dangling before we even read is_expired().
// Releasing the lock between the get and the remove is not safe, even with null checks:
// a freed pointer is still non-null; dereferencing it is UB.
//
// The lock must be held for the entire sequence: get pointer → inspect → conditionally remove.
std::optional<std::string> ChronoCache::get(const std::string& key) 
{
    std::optional<std::string> result;
    kv_store.check_and_remove(key, [&](CacheEntry* e) {
        if (!e) return false;
        if (e->is_expired()) return true;
        result = e->value;
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
    kv_store.check_and_remove(key, [&](CacheEntry* e) {
        if (!e) return false;
        if (e->is_expired()) return true;
        e->expires_at = CacheEntry::Clock::now() + ttl;
        current_value = e->value;
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
    kv_store.check_and_remove(key, [&](CacheEntry* e) {
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
    kv_store.check_and_remove(key, [&](CacheEntry* e) {
        if (!e) return false;
        if (e->is_expired()) return true;
        found = true; 
        if (e->expires_at.has_value()) {
            e->expires_at = std::nullopt;
            current_value = e->value;
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
