#ifndef CHRONO_CACHE_SRC_CPP_STORE_HASH_MAP_H
#define CHRONO_CACHE_SRC_CPP_STORE_HASH_MAP_H

#include <mutex>
#include <optional>
#include <shared_mutex>
#include <vector>

#include "store.h"

template<typename T_key, typename T_value>
class ChronCacheHashMap {
private:
    KVStore<T_key, T_value> kv_store;

    static constexpr int NUM_SLICES = 16;
    mutable std::shared_mutex resize_lock;
    mutable std::vector<std::shared_mutex> slice_locks;

    int compute_slice_index(int bucket_index) const {
        return bucket_index % NUM_SLICES;
    }

    struct ReadLock {
        std::shared_lock<std::shared_mutex> resize;
        std::shared_lock<std::shared_mutex> slice;
    };
    struct WriteLock {
        std::shared_lock<std::shared_mutex> resize;
        std::unique_lock<std::shared_mutex> slice;
    };

    ReadLock acquire_read_lock(const T_key& key) const {
        std::shared_lock<std::shared_mutex> rl(resize_lock);
        int bucket = kv_store.compute_bucket_index_for_key(key);
        int slice = compute_slice_index(bucket);
        return { std::move(rl), std::shared_lock<std::shared_mutex>(slice_locks[slice]) };
    }
    WriteLock acquire_write_lock(const T_key& key) const {
        std::shared_lock<std::shared_mutex> rl(resize_lock);
        int bucket = kv_store.compute_bucket_index_for_key(key);
        int slice = compute_slice_index(bucket);
        return { std::move(rl), std::unique_lock<std::shared_mutex>(slice_locks[slice]) };
    }

    void maybe_resize() {
        if (kv_store.needs_resize()) {
            std::unique_lock guard(resize_lock);
            if (kv_store.needs_resize()) kv_store.resize(kv_store.get_capacity() * 2);
        }
    }

public:
    ChronCacheHashMap(int capacity, double load_factor_threshold = 0.75)
        : kv_store(capacity, load_factor_threshold)
        , slice_locks(NUM_SLICES) {}

    ~ChronCacheHashMap() = default;

    ChronCacheHashMap(const ChronCacheHashMap&) = delete;
    ChronCacheHashMap& operator=(const ChronCacheHashMap&) = delete;
    ChronCacheHashMap(ChronCacheHashMap&&) = delete;
    ChronCacheHashMap& operator=(ChronCacheHashMap&&) = delete;

    bool set(const T_key& key, const T_value& value) {
        return set(key, value, []() {});
    }

    template<typename PostFn>
    bool set(const T_key& key, const T_value& value, PostFn&& post_fn) {
        bool resize_needed;
        {
            auto lock = acquire_write_lock(key);
            kv_store.set(key, value);
            std::forward<PostFn>(post_fn)();
            resize_needed = kv_store.needs_resize();
        }
        if (resize_needed) maybe_resize();
        return true;
    }

    T_value* get_ptr(const T_key& key) {
        auto lock = acquire_read_lock(key);
        return kv_store.get_ptr(key);
    }

    const T_value* get_ptr(const T_key& key) const {
        auto lock = acquire_read_lock(key);
        return kv_store.get_ptr(key);
    }

    std::optional<T_value> get(const T_key& key) {
        auto lock = acquire_read_lock(key);
        return kv_store.get(key);
    }

    std::optional<T_value> get(const T_key& key) const {
        auto lock = acquire_read_lock(key);
        return kv_store.get(key);
    }

    T_value* get_or_add(const T_key& key) {
        bool resize_needed;
        T_value* result;
        {
            auto lock = acquire_write_lock(key);
            result = kv_store.get_or_add(key);
            resize_needed = kv_store.needs_resize();
        }
        if (resize_needed) maybe_resize();
        return result;
    }

    bool remove(const T_key& key) {
        return remove(key, []() {});
    }

    template<typename PostFn>
    bool remove(const T_key& key, PostFn&& post_fn) {
        auto lock = acquire_write_lock(key);
        bool found = kv_store.remove(key);
        if (found) std::forward<PostFn>(post_fn)();
        return found;
    }

    void resize(int new_capacity) {
        std::unique_lock resize_guard(resize_lock);
        kv_store.resize(new_capacity);
    }

    template<typename Fn>
    void process_and_remove_if(const T_key& key, Fn&& cond_fn) {
        auto lock = acquire_write_lock(key);
        T_value* entry = kv_store.get_ptr(key);
        if (std::forward<Fn>(cond_fn)(entry) && entry != nullptr) {
            kv_store.remove(key);
        }
    }
};

#endif
