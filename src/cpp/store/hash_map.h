#ifndef CHRONO_CACHE_SRC_CPP_STORE_HASH_MAP_H
#define CHRONO_CACHE_SRC_CPP_STORE_HASH_MAP_H

#include <string>
#include <vector>

#include "node.h"

template<typename T_key, typename T_value>
class ChronCacheHashMap {
    private:
    int capacity;

    std::vector<ChronCacheNode<T_key, T_value>*> bucketHeadNode; 
    std::vector<ChronCacheNode<T_key, T_value>*> bucketTailNode;

    // Flattened global list: bucket 0 nodes -> bucket 1 nodes -> ... -> bucket N nodes
    ChronCacheNode<T_key, T_value>* globalHead = nullptr;
    ChronCacheNode<T_key, T_value>* globalTail = nullptr;

    public:
    ChronCacheHashMap(int capacity);

    bool set(const T_key& key, const T_value& value);    
    T_value get(const T_key& key) const;
    bool remove(const T_key& key);

    int get_bucket_index(const T_key& key) const;
};

// #include "hash_map.cpp"

#endif