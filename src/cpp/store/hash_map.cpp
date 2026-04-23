#include "hash_map.h"
#include "node.h"


template<typename T_key, typename T_value>
ChronCacheHashMap<T_key, T_value>::ChronCacheHashMap(int capacity)
    : capacity(capacity),
      bucketHeadNode(capacity, nullptr),
      bucketTailNode(capacity, nullptr) {
};


template<typename T_key, typename T_value>
bool ChronCacheHashMap<T_key, T_value>::set(const T_key& key, const T_value& value) {
    ChronCacheNode<T_key, T_value>* newNode = new ChronCacheNode<T_key, T_value>(key, value);

    int bucketIndex = get_bucket_index(key);
    if(bucketIndex >= (int)this->bucketHeadNode.size()) {
        throw std::runtime_error("Bucket index out of range");
    }

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
    return true;
}


template<typename T_key, typename T_value>
T_value ChronCacheHashMap<T_key, T_value>::get(const T_key& key) const {
    int bucketIndex = get_bucket_index(key);
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
    int bucketIndex = get_bucket_index(key);

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
            return true;
        }

        prevInBucket = current;
        current = current->next_in_bucket;
    }

    return false;
}

template<typename T_key, typename T_value>
int ChronCacheHashMap<T_key, T_value>::get_bucket_index(const T_key& key) const {
    return int(ChronCacheHashKey<T_key>(key).get() % ((size_t)capacity)); 
}