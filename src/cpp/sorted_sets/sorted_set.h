#ifndef SORTED_SET_H
#define SORTED_SET_H

#include <optional>
#include <string>
#include <unordered_map>

#include "../kv_store/hash_map.h"
#include "tree.h"

static constexpr int SORTED_SET_INITIAL_MAP_CAPACITY = 16;

struct SortedSet {
    AVLTree tree;
    ChronCacheHashMap<std::string, double> member_score;

    SortedSet();
};

class SortedSetsAPI {
    std::unordered_map<std::string, SortedSet> sorted_set_store;

    SortedSet& get_or_create_set(const std::string& key);
    const SortedSet* find_set(const std::string& key) const;

public:
    SortedSetsAPI();

    bool zadd(const std::string& key, double score, const std::string& member);

    std::optional<double> zscore(const std::string& key, const std::string& member) const;

    bool zrem(const std::string& key, const std::string& member);

    std::optional<int> zrank(const std::string& key, const std::string& member) const;
};

#endif
