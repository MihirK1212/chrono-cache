#ifndef CHRONO_CACHE_SRC_CPP_STORE_HASH_MAP_H
#define CHRONO_CACHE_SRC_CPP_STORE_HASH_MAP_H

#include <optional>
#include <string>
#include <vector>

#include "node.h"
#include "metrics.h"

template<typename T_key, typename T_value>
class ChronCacheHashMap {
    private:
    using Node = ChronCacheNode<T_key, T_value>;

    int capacity;
    double load_factor_threshold;
    ChronCacheHashMapMetrics metrics;

    std::vector<Node*> bucket_heads;
    std::vector<Node*> bucket_tails;
    std::vector<int> bucket_chain_lengths;

    // Flattened global list: bucket 0 nodes -> bucket 1 nodes -> ... -> bucket N nodes
    Node* global_head = nullptr;
    Node* global_tail = nullptr;

    size_t compute_hash(const T_key& key) const;
    int compute_bucket_index(size_t hashed_key) const;

    Node* find_in_bucket(int bucket_index, const T_key& key) const;
    void unlink_from_bucket_chain(Node* node, Node* prev_in_bucket, int bucket_index);

    Node* find_prev_bucket_tail(int bucket_index) const;
    Node* find_next_bucket_head(int bucket_index) const;
    void link_into_global_chain(Node* node, Node* prev, Node* next);
    void unlink_from_global_chain(Node* node);

    void insert_into_bucket(int bucket_index, Node* node);
    Node* insert_node(const T_key& key, const T_value* value);
    void reset_to_capacity(int capacity);
    void destroy_all_nodes();

    public:
    ChronCacheHashMap(int capacity, double load_factor_threshold = 0.75);
    ~ChronCacheHashMap();

    ChronCacheHashMap(const ChronCacheHashMap&) = delete;
    ChronCacheHashMap& operator=(const ChronCacheHashMap&) = delete;
    ChronCacheHashMap(ChronCacheHashMap&&) = delete;
    ChronCacheHashMap& operator=(ChronCacheHashMap&&) = delete;

    bool set(const T_key& key, const T_value& value);
    T_value* get_ptr(const T_key& key);
    const T_value* get_ptr(const T_key& key) const;
    std::optional<T_value> get(const T_key& key);
    std::optional<T_value> get(const T_key& key) const;
    T_value* get_or_add(const T_key& key);
    bool remove(const T_key& key);
    void resize(int new_capacity);
};

template<typename T_key, typename T_value>
ChronCacheHashMap<T_key, T_value>::ChronCacheHashMap(int capacity_val, double load_factor_threshold_val)
    : capacity(0)
    , load_factor_threshold(load_factor_threshold_val)
    , metrics(capacity_val) {
    reset_to_capacity(capacity_val);
}

template<typename T_key, typename T_value>
ChronCacheHashMap<T_key, T_value>::~ChronCacheHashMap() {
    destroy_all_nodes();
}

template<typename T_key, typename T_value>
bool ChronCacheHashMap<T_key, T_value>::set(const T_key& key, const T_value& value) {
    int bucket_index = compute_bucket_index(compute_hash(key));

    Node* existing = find_in_bucket(bucket_index, key);
    if (existing != nullptr) {
        existing->value = value;
        return true;
    }

    insert_node(key, &value);
    return true;
}

template<typename T_key, typename T_value>
T_value* ChronCacheHashMap<T_key, T_value>::get_ptr(const T_key& key) {
    int bucket_index = compute_bucket_index(compute_hash(key));
    Node* node = find_in_bucket(bucket_index, key);
    if (node != nullptr) {
        return &node->value;
    }
    return nullptr;
}

template<typename T_key, typename T_value>
const T_value* ChronCacheHashMap<T_key, T_value>::get_ptr(const T_key& key) const {
    return const_cast<ChronCacheHashMap*>(this)->get_ptr(key);
}

template<typename T_key, typename T_value>
std::optional<T_value> ChronCacheHashMap<T_key, T_value>::get(const T_key& key) {
    T_value* ptr = get_ptr(key);
    if (ptr != nullptr) {
        return *ptr;
    }
    return std::nullopt;
}

template<typename T_key, typename T_value>
std::optional<T_value> ChronCacheHashMap<T_key, T_value>::get(const T_key& key) const {
    const T_value* ptr = get_ptr(key);
    if (ptr != nullptr) {
        return *ptr;
    }
    return std::nullopt;
}

template<typename T_key, typename T_value>
T_value* ChronCacheHashMap<T_key, T_value>::get_or_add(const T_key& key) {
    T_value* existing = get_ptr(key);
    if (existing != nullptr) {
        return existing;
    }

    return &insert_node(key, nullptr)->value;
}

template<typename T_key, typename T_value>
bool ChronCacheHashMap<T_key, T_value>::remove(const T_key& key) {
    int bucket_index = compute_bucket_index(compute_hash(key));

    // Traversal inline because we need prev_in_bucket for singly-linked unlink
    Node* current = bucket_heads[bucket_index];
    Node* prev_in_bucket = nullptr;

    while (current != nullptr) {
        if (current->key == key) {
            unlink_from_bucket_chain(current, prev_in_bucket, bucket_index);
            unlink_from_global_chain(current);
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
void ChronCacheHashMap<T_key, T_value>::resize(int new_capacity) {
    if (new_capacity <= capacity) {
        throw std::runtime_error("New capacity must be greater than current capacity");
    }
    bool is_power_of_2 = (new_capacity > 0) && ((new_capacity & (new_capacity - 1)) == 0);
    if (!is_power_of_2) {
        throw std::runtime_error("New capacity must be a power of 2");
    }

    Node* current = global_head;

    int old_capacity = capacity;
    metrics.resize_event_triggered(old_capacity, new_capacity);
    reset_to_capacity(new_capacity);

    while (current != nullptr) {
        Node* next = current->next_global; // save before clear_pointers() nullifies it
        current->clear_pointers();
        int bucket_index = compute_bucket_index(current->get_hashed_key());
        insert_into_bucket(bucket_index, current);
        current = next;
    }
}


template<typename T_key, typename T_value>
size_t ChronCacheHashMap<T_key, T_value>::compute_hash(const T_key& key) const {
    return ChronCacheHashKey<T_key>(key).get();
}

template<typename T_key, typename T_value>
int ChronCacheHashMap<T_key, T_value>::compute_bucket_index(size_t hashed_key) const {
    /*
        Optimization trick:
        since capacity is a power of 2, bucket_index = hash % capacity = (hash & (capacity - 1))
    */
    return static_cast<int>(hashed_key & (static_cast<size_t>(capacity) - 1));
}

template<typename T_key, typename T_value>
ChronCacheNode<T_key, T_value>* ChronCacheHashMap<T_key, T_value>::find_in_bucket(
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
void ChronCacheHashMap<T_key, T_value>::unlink_from_bucket_chain(
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
ChronCacheNode<T_key, T_value>* ChronCacheHashMap<T_key, T_value>::find_prev_bucket_tail(
    int bucket_index
) const {
    for (int i = bucket_index - 1; i >= 0; i--) {
        if (bucket_tails[i] != nullptr) {
            return bucket_tails[i];
        }
    }
    return nullptr;
}

template<typename T_key, typename T_value>
ChronCacheNode<T_key, T_value>* ChronCacheHashMap<T_key, T_value>::find_next_bucket_head(
    int bucket_index
) const {
    for (int i = bucket_index + 1; i < capacity; i++) {
        if (bucket_heads[i] != nullptr) {
            return bucket_heads[i];
        }
    }
    return nullptr;
}

template<typename T_key, typename T_value>
void ChronCacheHashMap<T_key, T_value>::link_into_global_chain(
    Node* node, Node* prev, Node* next
) {
    node->prev_global = prev;
    node->next_global = next;

    if (prev != nullptr) {
        prev->next_global = node;
    } else {
        global_head = node;
    }

    if (next != nullptr) {
        next->prev_global = node;
    } else {
        global_tail = node;
    }
}

template<typename T_key, typename T_value>
void ChronCacheHashMap<T_key, T_value>::unlink_from_global_chain(Node* node) {
    if (node->prev_global != nullptr) {
        node->prev_global->next_global = node->next_global;
    } else {
        global_head = node->next_global;
    }

    if (node->next_global != nullptr) {
        node->next_global->prev_global = node->prev_global;
    } else {
        global_tail = node->prev_global;
    }
}

template<typename T_key, typename T_value>
ChronCacheNode<T_key, T_value>* ChronCacheHashMap<T_key, T_value>::insert_node(
    const T_key& key, const T_value* value
) {
    Node* new_node = (value != nullptr) ? new Node(key, *value) : new Node(key);
    int bucket_index = compute_bucket_index(compute_hash(key));
    insert_into_bucket(bucket_index, new_node);

    if (metrics.get_load_factor() > load_factor_threshold) {
        resize(capacity * 2);
    }

    return new_node;
}

template<typename T_key, typename T_value>
void ChronCacheHashMap<T_key, T_value>::insert_into_bucket(int bucket_index, Node* node) {
    bool is_collision = (bucket_heads[bucket_index] != nullptr);

    if (!is_collision) {
        bucket_heads[bucket_index] = node;

        Node* prev = find_prev_bucket_tail(bucket_index);
        Node* next = find_next_bucket_head(bucket_index);
        link_into_global_chain(node, prev, next);
    } else {
        Node* old_tail = bucket_tails[bucket_index];
        old_tail->next_in_bucket = node;
        link_into_global_chain(node, old_tail, old_tail->next_global);
    }

    bucket_tails[bucket_index] = node;
    bucket_chain_lengths[bucket_index]++;
    metrics.new_element_event(is_collision, bucket_chain_lengths[bucket_index]);
}

template<typename T_key, typename T_value>
void ChronCacheHashMap<T_key, T_value>::reset_to_capacity(int capacity_val) {
    bool is_power_of_2 = (capacity_val > 0) && ((capacity_val & (capacity_val - 1)) == 0);
    if (!is_power_of_2) {
        throw std::runtime_error("Capacity must be a power of 2");
    }

    capacity = capacity_val;
    global_head = nullptr;
    global_tail = nullptr;
    bucket_heads.assign(capacity, nullptr);
    bucket_tails.assign(capacity, nullptr);
    bucket_chain_lengths.assign(capacity, 0);
}

template<typename T_key, typename T_value>
void ChronCacheHashMap<T_key, T_value>::destroy_all_nodes() {
    Node* current = global_head;
    while (current != nullptr) {
        Node* next = current->next_global;
        delete current;
        current = next;
    }
    global_head = nullptr;
    global_tail = nullptr;
}

#endif
