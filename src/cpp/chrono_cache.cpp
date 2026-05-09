#include "chrono_cache.h"

ChronoCache::ChronoCache()
    : kv_store(CHRONO_CACHE_INITIAL_CAPACITY)
    , sorted_sets() {}

bool ChronoCache::set(const std::string& key, const std::string& value,
                      std::optional<std::chrono::milliseconds> ttl) {
    CacheEntry entry = ttl.has_value()
        ? CacheEntry(value, *ttl)
        : CacheEntry(value);
    return kv_store.set(key, entry);
}

std::optional<std::string> ChronoCache::get(const std::string& key) {
    CacheEntry* entry = kv_store.get_ptr(key);
    if (entry == nullptr) return std::nullopt;
    if (entry->is_expired()) {
        kv_store.remove(key);
        return std::nullopt;
    }
    return entry->value;
}

bool ChronoCache::del(const std::string& key) {
    return kv_store.remove(key);
}

bool ChronoCache::expire(const std::string& key, std::chrono::milliseconds ttl) {
    CacheEntry* entry = kv_store.get_ptr(key);
    if (entry == nullptr) return false;
    if (entry->is_expired()) {
        kv_store.remove(key);
        return false;
    }
    entry->expires_at = CacheEntry::Clock::now() + ttl;
    return true;
}

long long ChronoCache::pttl(const std::string& key) {
    CacheEntry* entry = kv_store.get_ptr(key);
    if (entry == nullptr) return -2;
    if (entry->is_expired()) {
        kv_store.remove(key);
        return -2;
    }
    if (!entry->has_ttl()) return -1;
    auto remaining = entry->remaining_ttl();
    return remaining->count();
}

bool ChronoCache::persist(const std::string& key) {
    CacheEntry* entry = kv_store.get_ptr(key); 
    if(entry == nullptr) return false; 
    if(entry->is_expired()) {
        kv_store.remove(key); 
        return false;  
    }
    entry->expires_at = std::nullopt; 
    return true; 
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
