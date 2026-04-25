#ifndef CHRONO_CACHE_SRC_CPP_STORE_HASH_MAP_H
#define CHRONO_CACHE_SRC_CPP_STORE_HASH_MAP_H

#include <string>
#include <vector>

#include "node.h"
#include "metrics.h"

template<typename T_key, typename T_value>
class ChronCacheHashMap {
    private:
    int capacity;
    ChronCacheHashMapMetrics metrics;

    double load_factor_threshold = 0.75;

    std::vector<ChronCacheNode<T_key, T_value>*> bucketHeadNode; 
    std::vector<ChronCacheNode<T_key, T_value>*> bucketTailNode;
    std::vector<int> bucketChainLength;

    // Flattened global list: bucket 0 nodes -> bucket 1 nodes -> ... -> bucket N nodes
    ChronCacheNode<T_key, T_value>* globalHead = nullptr;
    ChronCacheNode<T_key, T_value>* globalTail = nullptr;

    public:
    ChronCacheHashMap(int capacity, double load_factor_threshold = 0.75);

    void reset_map_to_capacity(int capacity);

    bool insert_into_bucket(int bucketIndex, ChronCacheNode<T_key, T_value>* newNode);

    bool set(const T_key& key, const T_value& value);    
    T_value get(const T_key& key) const;
    bool remove(const T_key& key);

    void resize(int new_capacity);

    int get_hashed_key(const T_key& key) const;
    int get_bucket_index(const size_t& hashed_key) const;
};

template<typename T_key, typename T_value>
ChronCacheHashMap<T_key, T_value>::ChronCacheHashMap(int capacity, double load_factor_threshold)
    : metrics(capacity) {
    this->load_factor_threshold = load_factor_threshold;
    this->reset_map_to_capacity(capacity);
}

template<typename T_key, typename T_value>
void ChronCacheHashMap<T_key, T_value>::reset_map_to_capacity(int capacity) {
    bool is_power_of_2 = (capacity > 0) && ((capacity & (capacity - 1)) == 0);
    if (!is_power_of_2) {
        throw std::runtime_error("Capacity must be a power of 2");
    }
    this->capacity = capacity;

    this->globalHead = nullptr;
    this->globalTail = nullptr;
    this->bucketHeadNode = std::vector<ChronCacheNode<T_key, T_value>*>(capacity, nullptr);
    this->bucketTailNode = std::vector<ChronCacheNode<T_key, T_value>*>(capacity, nullptr);
    this->bucketChainLength = std::vector<int>(capacity, 0);
}

template<typename T_key, typename T_value>
bool ChronCacheHashMap<T_key, T_value>::insert_into_bucket(
    int bucketIndex, ChronCacheNode<T_key, T_value>* newNode
) {
    
    bool is_collision = false;

    if (this->bucketHeadNode[bucketIndex] == nullptr) {
        // empty bucket
        this->bucketHeadNode[bucketIndex] = newNode;

        // tail of the closest non-empty bucket to the left
        ChronCacheNode<T_key, T_value>* prevTail = nullptr;
        for (int i = bucketIndex - 1; i >= 0; i--) {
            if (this->bucketTailNode[i] != nullptr) {
                prevTail = this->bucketTailNode[i];
                break;
            }
        }

        // head of the closest non-empty bucket to the right
        ChronCacheNode<T_key, T_value>* nextHead = nullptr;
        for (int i = bucketIndex + 1; i < capacity; i++) {
            if (this->bucketHeadNode[i] != nullptr) {
                nextHead = this->bucketHeadNode[i];
                break;
            }
        }

        newNode->prev_global = prevTail;
        newNode->next_global = nextHead;

        if (prevTail != nullptr) {
            prevTail->next_global = newNode;
        }
        else {
            globalHead = newNode;
        }

        if (nextHead != nullptr) {
            nextHead->prev_global = newNode;
        }

        else {
            globalTail = newNode;
        }
    } else {
        is_collision = true;

        // bucket already has nodes 
        ChronCacheNode<T_key, T_value>* oldTail = this->bucketTailNode[bucketIndex];

        oldTail->next_in_bucket = newNode;

        newNode->prev_global = oldTail;
        newNode->next_global = oldTail->next_global;

        if (oldTail->next_global != nullptr) {
            oldTail->next_global->prev_global = newNode;
        }
        else {
            globalTail = newNode;
        }

        oldTail->next_global = newNode;
    }

    this->bucketTailNode[bucketIndex] = newNode;
    this->bucketChainLength[bucketIndex]++;
    this->metrics.new_element_event(is_collision, this->bucketChainLength[bucketIndex]);

    return true;
}

template<typename T_key, typename T_value>
bool ChronCacheHashMap<T_key, T_value>::set(const T_key& key, const T_value& value) {
    int bucketIndex = get_bucket_index(get_hashed_key(key));
    if(bucketIndex >= (int)this->bucketHeadNode.size()) {
        throw std::runtime_error("Bucket index out of range");
    }

    // If key already exists, update in place
    ChronCacheNode<T_key, T_value>* existing = this->bucketHeadNode[bucketIndex];
    while (existing != nullptr) {
        if (existing->key == key) {
            existing->value = value;
            return true;
        }
        existing = existing->next_in_bucket;
    }

    ChronCacheNode<T_key, T_value>* newNode = new ChronCacheNode<T_key, T_value>(key, value);

    this->insert_into_bucket(bucketIndex, newNode);

    // resize if load factor is greater than the threshold
    double load_factor = this->metrics.get_load_factor();
    if (load_factor > this->load_factor_threshold) {
        this->resize(this->capacity * 2);
    }

    return true;
}


template<typename T_key, typename T_value>
T_value ChronCacheHashMap<T_key, T_value>::get(const T_key& key) const {
    int bucketIndex = get_bucket_index(get_hashed_key(key));
    if(bucketIndex >= (int)this->bucketHeadNode.size()) {
        throw std::runtime_error("Bucket index out of range");
    }
    ChronCacheNode<T_key, T_value>* current = this->bucketHeadNode[bucketIndex];

    while (current != nullptr) {
        if (current->key == key) {
            return current->value;
        }
        current = current->next_in_bucket;
    }

    throw std::runtime_error("Key not found");
}



template<typename T_key, typename T_value>
bool ChronCacheHashMap<T_key, T_value>::remove(const T_key& key) {
    int bucketIndex = get_bucket_index(get_hashed_key(key));

    if(bucketIndex >= (int)this->bucketHeadNode.size()) {
        throw std::runtime_error("Bucket index out of range");
    }

    ChronCacheNode<T_key, T_value>* current = this->bucketHeadNode[bucketIndex];
    ChronCacheNode<T_key, T_value>* prevInBucket = nullptr;

    while (current != nullptr) {
        if (current->key == key) {
            // unlink from bucket chain
            if (prevInBucket != nullptr) {
                prevInBucket->next_in_bucket = current->next_in_bucket;
            }

            else {
                this->bucketHeadNode[bucketIndex] = current->next_in_bucket;
            }

            if (current->next_in_bucket == nullptr) {
                this->bucketTailNode[bucketIndex] = prevInBucket;
            }

            // unlink from global chain
            if (current->prev_global != nullptr) {
                current->prev_global->next_global = current->next_global;
            }

            else {
                globalHead = current->next_global;
            }

            if (current->next_global != nullptr) {
                current->next_global->prev_global = current->prev_global;
            }

            else {
                globalTail = current->prev_global;
            }

            delete current;
            this->bucketChainLength[bucketIndex]--;
            this->metrics.remove_element_event();
            return true;
        }

        prevInBucket = current;
        current = current->next_in_bucket;
    }

    return false;
}

template<typename T_key, typename T_value> 
void ChronCacheHashMap<T_key, T_value>::resize(int new_capacity) {
    if (new_capacity <= this->capacity) {
        throw std::runtime_error("New capacity is less than the current capacity");
    }
    bool is_power_of_2 = (new_capacity > 0) && ((new_capacity & (new_capacity - 1)) == 0);
    if (!is_power_of_2) {
        throw std::runtime_error("New capacity must be a power of 2");
    }

    // DANGER: the nodes of the global chain for some time exist only as independent nodes outside of the hash map
    ChronCacheNode<T_key, T_value>* current = this->globalHead;
    
    // reset everything to the new capacity
    int old_capacity = this->capacity;
    this->metrics.resize_event_triggered(old_capacity, new_capacity);
    this->reset_map_to_capacity(new_capacity);

    while (current != nullptr) {
        current->clear_node_pointers(); // make the node independent of the global chain
        int bucketIndex = get_bucket_index(current->get_hashed_key());
        this->insert_into_bucket(bucketIndex, current);
        current = current->next_global;
    }
}   

template<typename T_key, typename T_value>
int ChronCacheHashMap<T_key, T_value>::get_hashed_key(const T_key& key) const {
    return ChronCacheHashKey<T_key>(key).get();
}

template<typename T_key, typename T_value>
int ChronCacheHashMap<T_key, T_value>::get_bucket_index(const size_t& hashed_key) const {
    /*
    Optimization trick:
    since capacity is a power of 2, bucket_index = hash % capacity = (hash & (capacity - 1))
    */
    return int(hashed_key & ((size_t)capacity - 1)); 
}

#endif