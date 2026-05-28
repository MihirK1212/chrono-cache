#ifndef CHRONO_CACHE_SRC_CPP_STORE_STORE_H
#define CHRONO_CACHE_SRC_CPP_STORE_STORE_H

#include <optional>
#include <vector>
#include <stdexcept>

#include "node.h"
#include "metrics.h"
#include "hash_key.h"

template<typename T_key, typename T_value>
class KVStore {
private:
    using Node = ChronCacheNode<T_key, T_value>;

    int capacity;
    double load_factor_threshold;
    ChronCacheHashMapMetrics metrics;

    std::vector<Node*> bucket_heads;
    std::vector<Node*> bucket_tails;
    std::vector<int> bucket_chain_lengths;

    size_t compute_hash(const T_key& key) const;
    int compute_bucket_index(size_t hashed_key) const;

    Node* find_in_bucket(int bucket_index, const T_key& key) const;

    void unlink_from_bucket_chain(Node* node, Node* prev_in_bucket, int bucket_index);

    void insert_into_bucket(int bucket_index, Node* node);
    Node* insert_node_impl(Node* new_node, const T_key& key);
    Node* insert_node(const T_key& key);
    Node* insert_node(const T_key& key, const T_value& value);

    void reset_to_capacity(int capacity);
    void destroy_all_nodes();
    
    public:
    KVStore(int capacity, double load_factor_threshold = 0.75);
    ~KVStore();

    KVStore(const KVStore&) = delete;
    KVStore& operator=(const KVStore&) = delete;
    KVStore(KVStore&&) = delete;
    KVStore& operator=(KVStore&&) = delete;

    // public unguarded methods without any locking (not thread safe)
    bool set(const T_key& key, const T_value& value);
    T_value* get_ptr(const T_key& key);
    const T_value* get_ptr(const T_key& key) const;
    std::optional<T_value> get(const T_key& key);
    std::optional<T_value> get(const T_key& key) const;
    T_value* get_or_add(const T_key& key);
    bool remove(const T_key& key);
    void resize(int new_capacity);

    bool needs_resize() const;
    int get_capacity() const { return capacity; }

    int compute_bucket_index_for_key(const T_key& key) const {
        return compute_bucket_index(compute_hash(key));
    }
};

template<typename T_key, typename T_value>
KVStore<T_key, T_value>::KVStore(int capacity_val, double load_factor_threshold_val)
    : capacity(0)
    , load_factor_threshold(load_factor_threshold_val)
    , metrics(capacity_val) {
    reset_to_capacity(capacity_val);
}

template<typename T_key, typename T_value>
KVStore<T_key, T_value>::~KVStore() {
    destroy_all_nodes();
}

template<typename T_key, typename T_value>
size_t KVStore<T_key, T_value>::compute_hash(const T_key& key) const {
    return ChronCacheHashKey<T_key>(key).get();
}

template<typename T_key, typename T_value>
int KVStore<T_key, T_value>::compute_bucket_index(size_t hashed_key) const {
    // assuming capacity is a power of 2, so we can use bitwise AND to get the bucket index
    // x % n == x & (n - 1) when n is a power of 2
    return static_cast<int>(hashed_key & (static_cast<size_t>(capacity) - 1));
}

template<typename T_key, typename T_value>
ChronCacheNode<T_key, T_value>* KVStore<T_key, T_value>::find_in_bucket(
    int bucket_index, const T_key& key
) const {
    Node* current = bucket_heads[bucket_index];
    while (current != nullptr) {
        if (current->key == key) {
            return current;
        }
        current = current->next_in_bucket;
    }
    return nullptr;
}

template<typename T_key, typename T_value>
void KVStore<T_key, T_value>::unlink_from_bucket_chain(
    Node* node, Node* prev_in_bucket, int bucket_index
) {
    if (prev_in_bucket != nullptr) {
        prev_in_bucket->next_in_bucket = node->next_in_bucket;
    } else {
        bucket_heads[bucket_index] = node->next_in_bucket;
    }

    if (node->next_in_bucket == nullptr) {
        bucket_tails[bucket_index] = prev_in_bucket;
    }

    bucket_chain_lengths[bucket_index]--;
}

template<typename T_key, typename T_value>
ChronCacheNode<T_key, T_value>* KVStore<T_key, T_value>::insert_node_impl(
    Node* new_node, const T_key& key
) {
    int bucket_index = compute_bucket_index(compute_hash(key));
    insert_into_bucket(bucket_index, new_node);
    return new_node;
}

template<typename T_key, typename T_value>
ChronCacheNode<T_key, T_value>* KVStore<T_key, T_value>::insert_node(const T_key& key) {
    return insert_node_impl(new Node(key), key);
}

template<typename T_key, typename T_value>
ChronCacheNode<T_key, T_value>* KVStore<T_key, T_value>::insert_node(
    const T_key& key, const T_value& value
) {
    return insert_node_impl(new Node(key, value), key);
}

template<typename T_key, typename T_value>
void KVStore<T_key, T_value>::insert_into_bucket(int bucket_index, Node* node) {
    bool is_collision = (bucket_heads[bucket_index] != nullptr);

    if (!is_collision) {
        bucket_heads[bucket_index] = node;
    } else {
        Node* old_tail = bucket_tails[bucket_index];
        old_tail->next_in_bucket = node;
    }

    bucket_tails[bucket_index] = node;
    bucket_chain_lengths[bucket_index]++;
    metrics.new_element_event(is_collision, bucket_chain_lengths[bucket_index]);
}

template<typename T_key, typename T_value>
void KVStore<T_key, T_value>::reset_to_capacity(int capacity_val) {
    bool is_power_of_2 = (capacity_val > 0) && ((capacity_val & (capacity_val - 1)) == 0);
    if (!is_power_of_2) {
        throw std::runtime_error("Capacity must be a power of 2");
    }

    capacity = capacity_val;
    bucket_heads.assign(capacity, nullptr);
    bucket_tails.assign(capacity, nullptr);
    bucket_chain_lengths.assign(capacity, 0);
}

template<typename T_key, typename T_value>
void KVStore<T_key, T_value>::destroy_all_nodes() {
    for (int i = 0; i < capacity; ++i) {
        Node* current = bucket_heads[i];
        while (current != nullptr) {
            Node* next = current->next_in_bucket;
            delete current;
            current = next;
        }
    }
}

template<typename T_key, typename T_value>
bool KVStore<T_key, T_value>::needs_resize() const {
    return metrics.get_load_factor() > load_factor_threshold;
}

template<typename T_key, typename T_value>
bool KVStore<T_key, T_value>::set(const T_key& key, const T_value& value) {
    int bucket_index = compute_bucket_index(compute_hash(key));
    Node* existing = find_in_bucket(bucket_index, key);
    if (existing != nullptr) {
        existing->value = value;
        return true;
    }
    Node* new_node = new Node(key, value);
    insert_into_bucket(bucket_index, new_node);
    return true;
}

template<typename T_key, typename T_value>
T_value* KVStore<T_key, T_value>::get_ptr(const T_key& key) {
    int bucket_index = compute_bucket_index(compute_hash(key));
    Node* node = find_in_bucket(bucket_index, key);
    if (node != nullptr) return &node->value;
    return nullptr;
}

template<typename T_key, typename T_value>
const T_value* KVStore<T_key, T_value>::get_ptr(const T_key& key) const {
    return static_cast<const T_value*>(
        const_cast<KVStore<T_key, T_value>*>(this)->get_ptr(key)
    );
}

template<typename T_key, typename T_value>
std::optional<T_value> KVStore<T_key, T_value>::get(const T_key& key) {
    T_value* ptr = get_ptr(key);
    if (ptr != nullptr) return *ptr;
    return std::nullopt;
}

template<typename T_key, typename T_value>
std::optional<T_value> KVStore<T_key, T_value>::get(const T_key& key) const {
    const T_value* ptr = get_ptr(key);
    if (ptr != nullptr) return *ptr;
    return std::nullopt;
}

template<typename T_key, typename T_value>
T_value* KVStore<T_key, T_value>::get_or_add(const T_key& key) {
    int bucket_index = compute_bucket_index(compute_hash(key));
    Node* existing = find_in_bucket(bucket_index, key);
    if (existing != nullptr) return &existing->value;

    Node* new_node = new Node(key);
    insert_into_bucket(bucket_index, new_node);
    return &new_node->value;
}

template<typename T_key, typename T_value>
bool KVStore<T_key, T_value>::remove(const T_key& key) {
    int bucket_index = compute_bucket_index(compute_hash(key));
    Node* current = bucket_heads[bucket_index];
    Node* prev_in_bucket = nullptr;

    while (current != nullptr) {
        if (current->key == key) {
            unlink_from_bucket_chain(current, prev_in_bucket, bucket_index);
            delete current;
            metrics.remove_element_event();
            return true;
        }
        prev_in_bucket = current;
        current = current->next_in_bucket;
    }

    return false;
}

template<typename T_key, typename T_value>
void KVStore<T_key, T_value>::resize(int new_capacity) {
    if (new_capacity <= capacity) {
        throw std::runtime_error("New capacity must be greater than current capacity");
    }
    bool is_power_of_2 = (new_capacity > 0) && ((new_capacity & (new_capacity - 1)) == 0);
    if (!is_power_of_2) {
        throw std::runtime_error("New capacity must be a power of 2");
    }

    int old_capacity = capacity;
    std::vector<Node*> nodes;

    for (int i = 0; i < old_capacity; ++i) {
        Node* cur = bucket_heads[i];
        while (cur != nullptr) {
            nodes.push_back(cur);
            cur = cur->next_in_bucket;
        }
    }

    metrics.resize_event_triggered(old_capacity, new_capacity);
    reset_to_capacity(new_capacity);

    for (Node* node : nodes) {
        node->clear_pointers();
        int bucket_index = compute_bucket_index(node->get_hashed_key());
        insert_into_bucket(bucket_index, node);
    }
}

#endif

