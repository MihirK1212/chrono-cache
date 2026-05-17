#ifndef CHRONO_CACHE_SRC_CPP_STORE_HASH_MAP_H
#define CHRONO_CACHE_SRC_CPP_STORE_HASH_MAP_H

#include <mutex>
#include <optional>
#include <shared_mutex>
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

    static constexpr int NUM_SLICES = 16;
    mutable std::shared_mutex resize_lock;
    mutable std::vector<std::shared_mutex> slice_locks;
    mutable std::mutex global_chain_mutex;

    size_t compute_hash(const T_key& key) const;
    int compute_bucket_index(size_t hashed_key) const;
    int compute_slice_index(int bucket_index) const;

    Node* find_in_bucket(int bucket_index, const T_key& key) const;
    void unlink_from_bucket_chain(Node* node, Node* prev_in_bucket, int bucket_index);

    Node* find_prev_bucket_tail(int bucket_index) const;
    Node* find_next_bucket_head(int bucket_index) const;
    void link_into_global_chain(Node* node, Node* prev, Node* next);
    void unlink_from_global_chain(Node* node);

    void insert_into_bucket(int bucket_index, Node* node);
    Node* insert_node_impl(Node* new_node, const T_key& key);
    Node* insert_node(const T_key& key);
    Node* insert_node(const T_key& key, const T_value& value);
    void reset_to_capacity(int capacity);
    void destroy_all_nodes();
    void resize_impl(int new_capacity);// no locks in this method, assumes that the exclusive resize lock is held
    bool needs_resize() const;

    struct ReadLock {
        std::shared_lock<std::shared_mutex> resize;
        std::shared_lock<std::shared_mutex> slice;
    };
    struct WriteLock {
        std::shared_lock<std::shared_mutex> resize;
        std::unique_lock<std::shared_mutex> slice;
    };

    ReadLock acquire_read_lock(const T_key& key) const;
    WriteLock acquire_write_lock(const T_key& key) const;

    // core logic without locking, other than the global chain mutex
    // assumes that the caller has already acquired the appropriate slice and resize locks
    T_value* get_ptr_unguarded(const T_key& key);
    const T_value* get_ptr_unguarded(const T_key& key) const;
    bool set_unguarded(const T_key& key, const T_value& value);
    bool remove_unguarded(const T_key& key);
    T_value* get_or_add_unguarded(const T_key& key);

    public:
    ChronCacheHashMap(int capacity, double load_factor_threshold = 0.75);
    ~ChronCacheHashMap();

    ChronCacheHashMap(const ChronCacheHashMap&) = delete;
    ChronCacheHashMap& operator=(const ChronCacheHashMap&) = delete;
    ChronCacheHashMap(ChronCacheHashMap&&) = delete;
    ChronCacheHashMap& operator=(ChronCacheHashMap&&) = delete;

    bool set(const T_key& key, const T_value& value);
    template<typename PostFn>
    bool set(const T_key& key, const T_value& value, PostFn&& post_fn);
    T_value* get_ptr(const T_key& key);
    const T_value* get_ptr(const T_key& key) const;
    std::optional<T_value> get(const T_key& key);
    std::optional<T_value> get(const T_key& key) const;
    T_value* get_or_add(const T_key& key);
    bool remove(const T_key& key);
    template<typename PostFn>
    bool remove(const T_key& key, PostFn&& post_fn);
    void resize(int new_capacity);

    // Acquires a write lock for key, then calls fn(entry_ptr).
    // fn receives T_value* (nullptr if the key is not in the map).
    // If fn returns true and the entry exists, it is removed under the same lock.
    template<typename Fn>
    void get_and_remove_if(const T_key& key, Fn&& fn);
};

template<typename T_key, typename T_value>
ChronCacheHashMap<T_key, T_value>::ChronCacheHashMap(int capacity_val, double load_factor_threshold_val)
    : capacity(0)
    , load_factor_threshold(load_factor_threshold_val)
    , metrics(capacity_val)
    , slice_locks(NUM_SLICES) {
    reset_to_capacity(capacity_val);
}

template<typename T_key, typename T_value>
ChronCacheHashMap<T_key, T_value>::~ChronCacheHashMap() {
    destroy_all_nodes();
}

template<typename T_key, typename T_value>
typename ChronCacheHashMap<T_key, T_value>::ReadLock
ChronCacheHashMap<T_key, T_value>::acquire_read_lock(const T_key& key) const {
    std::shared_lock<std::shared_mutex> rl(resize_lock);
    int slice = compute_slice_index(compute_bucket_index(compute_hash(key)));
    return { std::move(rl), std::shared_lock<std::shared_mutex>(slice_locks[slice]) };
}

template<typename T_key, typename T_value>
typename ChronCacheHashMap<T_key, T_value>::WriteLock
ChronCacheHashMap<T_key, T_value>::acquire_write_lock(const T_key& key) const {
    std::shared_lock<std::shared_mutex> rl(resize_lock);
    int slice = compute_slice_index(compute_bucket_index(compute_hash(key)));
    return { std::move(rl), std::unique_lock<std::shared_mutex>(slice_locks[slice]) };
}

template<typename T_key, typename T_value>
bool ChronCacheHashMap<T_key, T_value>::set(const T_key& key, const T_value& value) {
    /*
    double-checked locking:
     1. a check (needs_resize()) is performed under the key's slice lock (shared resize_lock).
     2. if resize is needed, all locks are released, and an exclusive resize_lock is acquired.
     3. a second, more strict check (needs_resize()) is performed under the exclusive resize_lock
    */
    bool resize_needed;
    {
        auto lock = acquire_write_lock(key);
        set_unguarded(key, value);
        resize_needed = needs_resize();
    }
    if (resize_needed) {
        std::unique_lock resize_guard(resize_lock);
        if (needs_resize()) resize_impl(capacity * 2);
    }
    return true;
}

template<typename T_key, typename T_value>
template<typename PostFn>
bool ChronCacheHashMap<T_key, T_value>::set(
    const T_key& key, const T_value& value, PostFn&& post_fn
) {
    bool resize_needed;
    {
        auto lock = acquire_write_lock(key);
        set_unguarded(key, value);
        std::forward<PostFn>(post_fn)();   
        resize_needed = needs_resize();
    }
    if (resize_needed) {
        std::unique_lock resize_guard(resize_lock);
        if (needs_resize()) resize_impl(capacity * 2);
    }
    return true;
}

template<typename T_key, typename T_value>
T_value* ChronCacheHashMap<T_key, T_value>::get_ptr(const T_key& key) {
    auto lock = acquire_read_lock(key);
    return get_ptr_unguarded(key);
}

template<typename T_key, typename T_value>
const T_value* ChronCacheHashMap<T_key, T_value>::get_ptr(const T_key& key) const {
    auto lock = acquire_read_lock(key);
    return get_ptr_unguarded(key);
}

template<typename T_key, typename T_value>
std::optional<T_value> ChronCacheHashMap<T_key, T_value>::get(const T_key& key) {
    auto lock = acquire_read_lock(key);
    T_value* ptr = get_ptr_unguarded(key);
    if (ptr != nullptr) return *ptr;
    return std::nullopt;
}

template<typename T_key, typename T_value>
std::optional<T_value> ChronCacheHashMap<T_key, T_value>::get(const T_key& key) const {
    auto lock = acquire_read_lock(key);
    const T_value* ptr = get_ptr_unguarded(key);
    if (ptr != nullptr) return *ptr;
    return std::nullopt;
}

template<typename T_key, typename T_value>
T_value* ChronCacheHashMap<T_key, T_value>::get_or_add(const T_key& key) {
    /*
    double-checked locking:
     1. a check (needs_resize()) is performed under the key's slice lock (shared resize_lock).
     2. if resize is needed, all locks are released, and an exclusive resize_lock is acquired.
     3. a second, more strict check (needs_resize()) is performed under the exclusive resize_lock
    */
    bool resize_needed;
    T_value* result;
    {
        auto lock = acquire_write_lock(key);
        result = get_or_add_unguarded(key);
        resize_needed = needs_resize();
    }
    if (resize_needed) {
        std::unique_lock resize_guard(resize_lock);
        if (needs_resize()) resize_impl(capacity * 2);
    }
    return result;
}

template<typename T_key, typename T_value>
bool ChronCacheHashMap<T_key, T_value>::remove(const T_key& key) {
    auto lock = acquire_write_lock(key);
    return remove_unguarded(key);
}

template<typename T_key, typename T_value>
template<typename PostFn>
bool ChronCacheHashMap<T_key, T_value>::remove(const T_key& key, PostFn&& post_fn) {
    auto lock = acquire_write_lock(key);
    bool found = remove_unguarded(key);
    if (found) std::forward<PostFn>(post_fn)(); 
    return found;
}

template<typename T_key, typename T_value>
void ChronCacheHashMap<T_key, T_value>::resize(int new_capacity) {
    std::unique_lock resize_guard(resize_lock);
    resize_impl(new_capacity);
}


template<typename T_key, typename T_value>
T_value* ChronCacheHashMap<T_key, T_value>::get_ptr_unguarded(const T_key& key) {
    int bucket_index = compute_bucket_index(compute_hash(key));
    Node* node = find_in_bucket(bucket_index, key);
    if (node != nullptr) return &node->value;
    return nullptr;
}

template<typename T_key, typename T_value>
const T_value* ChronCacheHashMap<T_key, T_value>::get_ptr_unguarded(const T_key& key) const {
    return static_cast<const T_value*>(
        const_cast<ChronCacheHashMap<T_key, T_value>*>(this)->get_ptr_unguarded(key)
    );
}

template<typename T_key, typename T_value>
bool ChronCacheHashMap<T_key, T_value>::set_unguarded(const T_key& key, const T_value& value) {
    int bucket_index = compute_bucket_index(compute_hash(key));
    Node* existing = find_in_bucket(bucket_index, key);
    if (existing != nullptr) {
        existing->value = value;
        return true;
    }
    Node* new_node = new Node(key, value);
    {
        std::lock_guard gc_guard(global_chain_mutex);
        insert_into_bucket(bucket_index, new_node);
    }
    return true;
}

template<typename T_key, typename T_value>
bool ChronCacheHashMap<T_key, T_value>::remove_unguarded(const T_key& key) {
    int bucket_index = compute_bucket_index(compute_hash(key));
    Node* current = bucket_heads[bucket_index];
    Node* prev_in_bucket = nullptr;

    while (current != nullptr) {
        if (current->key == key) {
            unlink_from_bucket_chain(current, prev_in_bucket, bucket_index);
            {
                std::lock_guard gc_guard(global_chain_mutex);
                unlink_from_global_chain(current);
            }
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
T_value* ChronCacheHashMap<T_key, T_value>::get_or_add_unguarded(const T_key& key) {
    int bucket_index = compute_bucket_index(compute_hash(key));
    Node* existing = find_in_bucket(bucket_index, key);
    if (existing != nullptr) return &existing->value;

    Node* new_node = new Node(key);
    {
        std::lock_guard gc_guard(global_chain_mutex);
        insert_into_bucket(bucket_index, new_node);
    }
    return &new_node->value;
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
int ChronCacheHashMap<T_key, T_value>::compute_slice_index(int bucket_index) const {
    return bucket_index % NUM_SLICES;
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
ChronCacheNode<T_key, T_value>* ChronCacheHashMap<T_key, T_value>::insert_node_impl(
    Node* new_node, const T_key& key
) {
    int bucket_index = compute_bucket_index(compute_hash(key));
    insert_into_bucket(bucket_index, new_node);
    return new_node;
}

template<typename T_key, typename T_value>
ChronCacheNode<T_key, T_value>* ChronCacheHashMap<T_key, T_value>::insert_node(const T_key& key) {
    return insert_node_impl(new Node(key), key);
}

template<typename T_key, typename T_value>
ChronCacheNode<T_key, T_value>* ChronCacheHashMap<T_key, T_value>::insert_node(
    const T_key& key, const T_value& value
) {
    return insert_node_impl(new Node(key, value), key);
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

template<typename T_key, typename T_value>
void ChronCacheHashMap<T_key, T_value>::resize_impl(int new_capacity) {
    
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
        Node* next = current->next_global;
        current->clear_pointers();
        int bucket_index = compute_bucket_index(current->get_hashed_key());
        insert_into_bucket(bucket_index, current);
        current = next;
    }
}

template<typename T_key, typename T_value>
bool ChronCacheHashMap<T_key, T_value>::needs_resize() const {
    return metrics.get_load_factor() > load_factor_threshold;
}

template<typename T_key, typename T_value>
template<typename Fn>
void ChronCacheHashMap<T_key, T_value>::get_and_remove_if(const T_key& key, Fn&& fn) {
    auto lock = acquire_write_lock(key);
    T_value* entry = get_ptr_unguarded(key);
    if (std::forward<Fn>(fn)(entry) && entry != nullptr) {
        remove_unguarded(key);
    }
}

#endif
