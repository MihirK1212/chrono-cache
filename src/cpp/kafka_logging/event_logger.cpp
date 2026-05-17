#include <chrono>

#include "event_logger.h"

CacheEventLogger::CacheEventLogger(const std::string& brokers, const std::string& topic)
    : producer(brokers, topic) {}

void CacheEventLogger::set_seq_counters(std::unordered_map<std::string, uint64_t> last_applied_seq) {
    for (auto& [key, seq] : last_applied_seq) {
        seq_counters.set(key, seq);
    }
}

uint64_t CacheEventLogger::next_seq(const std::string& key) {
    /*
    at first glance, this seems like a race condtion because get and incr are not atomic. 
    but, the assumption is that next_seq will always be called inside log_<xyz> operations
    and log_<xyz> operations are called inside the post_fn or the cond_fn callbacks of the hash map
    and those callbacks are called while the stripe lock is held.
    the same key always returns the same stripe lock, so no other key can be accessed while the lock is held.
    hence, next_seq for a particular key is always called while its stripe lock is held, so no race condition exists.
    */
    uint64_t* ptr = seq_counters.get_or_add(key);
    return ++(*ptr);
}

int64_t CacheEventLogger::now_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

void CacheEventLogger::log_set(const std::string& key, const std::string& value, int64_t ttl_ms) {
    CacheEvent event;
    event.type      = EventType::SET;
    event.key       = key;
    event.value     = value;
    event.ttl_ms    = ttl_ms;
    event.seq       = next_seq(key);
    event.timestamp = now_ms();
    producer.produce_cache_event(event);
}

void CacheEventLogger::log_del(const std::string& key) {
    CacheEvent event;
    event.type      = EventType::DELETE;
    event.key       = key;
    event.ttl_ms    = 0;
    event.seq       = next_seq(key);
    event.timestamp = now_ms();
    producer.produce_cache_event(event);
}

void CacheEventLogger::log_expire(const std::string& key, const std::string& value, int64_t ttl_ms) {
    CacheEvent event;
    event.type      = EventType::EXPIRE;
    event.key       = key;
    event.value     = value;
    event.ttl_ms    = ttl_ms;
    event.seq       = next_seq(key);
    event.timestamp = now_ms();
    producer.produce_cache_event(event);
}

void CacheEventLogger::log_persist(const std::string& key, const std::string& value) {
    CacheEvent event;
    event.type      = EventType::PERSIST;
    event.key       = key;
    event.value     = value;
    event.ttl_ms    = 0;
    event.seq       = next_seq(key);
    event.timestamp = now_ms();
    producer.produce_cache_event(event);
}
