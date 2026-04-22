#ifndef CHRONO_CACHE_SRC_CPP_STORE_HASH_MAP_H
#define CHRONO_CACHE_SRC_CPP_STORE_HASH_MAP_H

#include <string>
#include <vector>

#include "node.h"

template<typename T_key, typename T_value>
class ChronCacheHashMap {
    private:
    int capacity;

    // bucketHeadNode[bucketIndex] stores the address of the head node of the `bucketIndex` bucket
    std::vector<ChronCacheNode<T_key, T_value>*> bucketHeadNode; 

    public:
    ChronCacheHashMap(int capacity);

    bool set(const T_key& key, const T_value& value);    
    T_value get(const T_key& key) const;
    bool remove(const T_key& key);

    int get_bucket_index(const T_key& key) const;
};

#endif