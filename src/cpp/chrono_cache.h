#ifndef CHRONO_CACHE_H
#define CHRONO_CACHE_H

#include <optional>
#include <string>

#include "kv_store/hash_map.h"
#include "sorted_sets/sorted_set.h"

static constexpr int CHRONO_CACHE_INITIAL_CAPACITY = 16;

class ChronoCache {
    ChronCacheHashMap<std::string, std::string> kv_store;
    SortedSetsAPI sorted_sets;

public:
    ChronoCache();

    bool set(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key) const;
    bool del(const std::string& key);

    bool zadd(const std::string& key, double score, const std::string& member);
    std::optional<double> zscore(const std::string& key, const std::string& member) const;
    bool zrem(const std::string& key, const std::string& member);
    std::optional<int> zrank(const std::string& key, const std::string& member) const;
};

#endif
