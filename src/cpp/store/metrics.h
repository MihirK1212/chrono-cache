#ifndef CHRONO_CACHE_SRC_CPP_STORE_METRICS_H
#define CHRONO_CACHE_SRC_CPP_STORE_METRICS_H

#include <stdexcept>

struct ChronCacheHashMapMetrics {
    int num_buckets;
    int num_entries; 
    int num_collisions; 
    int num_resizes; 
    int max_chain_length;
    int load_factor;

    ChronCacheHashMapMetrics(int num_buckets) : num_buckets(num_buckets), num_entries(0), num_collisions(0), num_resizes(0), max_chain_length(0), load_factor(0) {}
    ~ChronCacheHashMapMetrics() = default;

    void new_element_event(bool is_collision, int chain_length) {
        if (is_collision) {
            this->num_collisions++;
        }
        if (chain_length > this->max_chain_length) {
            this->max_chain_length = chain_length;
        }
        this->num_entries++;
        this->set_load_factor();
    }

    void remove_element_event() {
        this->num_entries--;
        this->set_load_factor();
    }

    void resize_event_triggered(int curr_capacity, int new_capacity) {
        if (this->num_buckets != curr_capacity) {
            throw std::runtime_error("Current capacity does not match the number of buckets");
        }

        this->num_buckets = new_capacity;
        this->num_entries = 0;
        this->num_collisions = 0;
        this->max_chain_length = 0;
        this->num_resizes++;

        this->set_load_factor();
    }

    int get_load_factor() const {
        return this->load_factor;
    }

    void set_load_factor() {
        this->load_factor = this->num_entries / this->num_buckets;
    }
};


#endif