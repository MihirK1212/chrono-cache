#ifndef CHRONO_CACHE_SRC_CPP_STORE_METRICS_H
#define CHRONO_CACHE_SRC_CPP_STORE_METRICS_H

#include <stdexcept>

struct ChronCacheHashMapMetrics {
    int num_buckets;
    int num_entries;
    int num_collisions;
    int num_resizes;
    int max_chain_length;
    double load_factor_val;

    ChronCacheHashMapMetrics(int num_buckets)
        : num_buckets(num_buckets)
        , num_entries(0)
        , num_collisions(0)
        , num_resizes(0)
        , max_chain_length(0)
        , load_factor_val(0.0) {}

    ~ChronCacheHashMapMetrics() = default;

    void new_element_event(bool is_collision, int chain_length) {
        if (is_collision) {
            num_collisions++;
        }
        if (chain_length > max_chain_length) {
            max_chain_length = chain_length;
        }
        num_entries++;
        recalculate_load_factor();
    }

    void remove_element_event() {
        num_entries--;
        recalculate_load_factor();
    }

    void resize_event_triggered(int curr_capacity, int new_capacity) {
        if (num_buckets != curr_capacity) {
            throw std::runtime_error("Current capacity does not match the number of buckets");
        }

        num_buckets = new_capacity;
        num_entries = 0;
        num_collisions = 0;
        max_chain_length = 0;
        num_resizes++;

        recalculate_load_factor();
    }

    double get_load_factor() const {
        return load_factor_val;
    }

    void recalculate_load_factor() {
        load_factor_val = static_cast<double>(num_entries) / static_cast<double>(num_buckets);
    }
};

#endif
