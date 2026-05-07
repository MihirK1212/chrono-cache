#ifndef CHRONO_CACHE_SRC_CPP_STORE_NODE_H
#define CHRONO_CACHE_SRC_CPP_STORE_NODE_H

#include "hash_key.h"

template<typename T_key, typename T_value>
struct ChronCacheNode {
    const T_key key;
    const size_t hashed_key;
    T_value value;

    ChronCacheNode* next_in_bucket;
    ChronCacheNode* prev_global;
    ChronCacheNode* next_global;

    ChronCacheNode(const T_key& key, const T_value& value)
        : key(key)
        , hashed_key(ChronCacheHashKey<T_key>(key).get())
        , value(value)
        , next_in_bucket(nullptr)
        , prev_global(nullptr)
        , next_global(nullptr) {}

    explicit ChronCacheNode(const T_key& key)
        : key(key)
        , hashed_key(ChronCacheHashKey<T_key>(key).get())
        , value()
        , next_in_bucket(nullptr)
        , prev_global(nullptr)
        , next_global(nullptr) {}

    ~ChronCacheNode() = default;

    const T_key& get_key() const {
        return key;
    }

    const size_t& get_hashed_key() const {
        return hashed_key;
    }

    const T_value& get_value() const {
        return value;
    }

    void clear_pointers() {
        next_in_bucket = nullptr;
        prev_global = nullptr;
        next_global = nullptr;
    }

    ChronCacheNode(const ChronCacheNode&) = delete;
    ChronCacheNode& operator=(const ChronCacheNode&) = delete;
    ChronCacheNode(ChronCacheNode&&) = delete;
    ChronCacheNode& operator=(ChronCacheNode&&) = delete;
};

#endif
