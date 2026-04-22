#ifndef CHRONO_CACHE_SRC_CPP_STORE_NODE_H
#define CHRONO_CACHE_SRC_CPP_STORE_NODE_H

#include "hash_key.h"

template<typename T_key, typename T_value>

struct ChronCacheNode {
    private:
    const T_key key;
    T_value value;

    ChronCacheNode* next_in_bucket; // Next node in the same bucket
    ChronCacheNode* prev_global;   // Previous node in global traversal
    ChronCacheNode* next_global;   // Next node in global traversal

    public:
    ChronCacheNode(const T_key& key, const T_value& value) : key(key), value(value) {
        next_in_bucket = nullptr;
        prev_global = nullptr;
        next_global = nullptr;
    }

    ~ChronCacheNode() = default;

    const T_key& get_key() const {
        return key;
    }

    const T_value& get_value() const {
        return value;
    }

    // purposley delete the copy and move constructors and operators
    // this prevents double free errors
    ChronCacheNode(const ChronCacheNode&) = delete; 
    ChronCacheNode& operator=(const ChronCacheNode&) = delete;
    ChronCacheNode(ChronCacheNode&&) = delete;
    ChronCacheNode& operator=(ChronCacheNode&&) = delete;
};

#endif