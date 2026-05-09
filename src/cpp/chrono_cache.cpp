#include "chrono_cache.h"

ChronoCache::ChronoCache()
    : kv_store(CHRONO_CACHE_INITIAL_CAPACITY)
    , sorted_sets() {}

bool ChronoCache::set(const std::string& key, const std::string& value) {
    return kv_store.set(key, value);
}

std::optional<std::string> ChronoCache::get(const std::string& key) const {
    return kv_store.get(key);
}

bool ChronoCache::del(const std::string& key) {
    return kv_store.remove(key);
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
