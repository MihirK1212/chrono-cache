#include <cstdlib>

#include "hash_map.h"
#include "node.h"


template<typename T_key, typename T_value>
ChronCacheHashMap<T_key, T_value>::ChronCacheHashMap(int capacity) : capacity(capacity) {
    bucketHeadNode = (ChronCacheNode<T_key, T_value>**)calloc(
        capacity, sizeof(ChronCacheNode<T_key, T_value>*)
    );
};


template<typename T_key, typename T_value>
bool ChronCacheHashMap<T_key, T_value>::set(const T_key& key, const T_value& value) {
    ChronCacheNode<T_key, T_value>* newNode = new ChronCacheNode<T_key, T_value>(key, value);

    int bucketIndex = get_bucket_index(key);
    if(bucketIndex >= this->bucketHeadNode.size()) {
        throw std::runtime_error("Bucket index out of range");
    }
    ChronCacheNode<T_key, T_value>* bucketHeadNode = this->bucketHeadNode[bucketIndex];

    // add new node to the end of the bucket
    if (bucketHeadNode == nullptr) {
        this->bucketHeadNode[bucketIndex] = newNode;
    }
    else {
        ChronCacheNode<T_key, T_value>* currentNode = bucketHeadNode;
        while (currentNode && currentNode->next_in_bucket != nullptr) {
            currentNode = currentNode->next_in_bucket;
        }
        currentNode->next_in_bucket = newNode;
    }

    return true;
}

template<typename T_key, typename T_value>
T_value ChronCacheHashMap<T_key, T_value>::get(const T_key& key) const {
    return T_value();
}

template<typename T_key, typename T_value>
bool ChronCacheHashMap<T_key, T_value>::remove(const T_key& key) {
    return true;
}

template<typename T_key, typename T_value>
int ChronCacheHashMap<T_key, T_value>::get_bucket_index(const T_key& key) const {
    return int(ChronCacheHashKey<T_key>(key).get() % ((size_t)capacity)); 
}