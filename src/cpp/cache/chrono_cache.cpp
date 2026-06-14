#include <stdexcept>
#include <unordered_map>

#include "chrono_cache.h"
#include "kafka_logging/event_consumer.h"


ChronoCache::ChronoCache(const CacheConfig& config)
    : state(ChronoCacheState::UNINITIALIZED)
    , hash_map(CHRONO_CACHE_INITIAL_CAPACITY)
    , sorted_sets()
    , disable_event_logging(config.disable_event_logging)
    , kafka_brokers(config.kafka_brokers)
    , kafka_topic(config.kafka_cache_events_topic) {}

bool ChronoCache::check_logging_allowed() const {
    if (disable_event_logging || state != ChronoCacheState::READY) {
        return false;
    }
    if (!cache_event_logger) {
        throw std::logic_error("Event logging is enabled but logger was not initialized");
    }
    return true;
}

bool ChronoCache::is_accepting_ops() const {
    return state == ChronoCacheState::READY || state == ChronoCacheState::REPLAYING;
}

void ChronoCache::check_accepting_ops() {
    if(!is_accepting_ops()) {
        throw std::runtime_error("ChronoCache is not ready to accept operations");
    }
}

bool ChronoCache::set(const std::string& key, const std::string& value, std::optional<std::chrono::milliseconds> ttl) 
{
    check_accepting_ops();

    CacheEntry entry = ttl.has_value()
        ? CacheEntry(value, *ttl)
        : CacheEntry(value);

    return hash_map.set(key, entry, [&]() {
        if (check_logging_allowed()) {
            cache_event_logger->log_set(key, value, ttl.has_value() ? ttl->count() : -1);
        }
    });
}

std::optional<std::string> ChronoCache::get(const std::string& key) 
{
    check_accepting_ops();

    std::optional<std::string> result;
    hash_map.process_and_remove_if(key, [&](CacheEntry* e) {
        if (!e) return false;
        if (e->is_expired()) return true;
        result = e->get_value();
        return false;
    });
    return result;
}

bool ChronoCache::del(const std::string& key) 
{
    check_accepting_ops();

    return hash_map.remove(key, [&]() {
        if (check_logging_allowed()) {
            cache_event_logger->log_del(key);
        }
    });
}

bool ChronoCache::expire(const std::string& key, std::chrono::milliseconds ttl) {
    check_accepting_ops();

    if(ttl.count() <= 0) {
        throw std::invalid_argument("TTL must be greater than 0");
    }

    bool updated_ttl = false;
    std::string current_value;
    hash_map.process_and_remove_if(key, [&](CacheEntry* e) {
        if (!e) return false;
        if (e->is_expired()) return true;
        e->update_ttl(ttl);
        current_value = e->get_value();
        updated_ttl = true;
        if (check_logging_allowed()) {
            cache_event_logger->log_expire(key, current_value, ttl.count());
        }
        return false;
    });

    return updated_ttl;
}

long long ChronoCache::pttl(const std::string& key) {
    check_accepting_ops();

    long long result = -2;
    hash_map.process_and_remove_if(key, [&](CacheEntry* e) {
        if (!e) return false;
        if (e->is_expired()) return true;
        result = e->has_ttl() ? e->remaining_ttl()->count() : -1;
        return false;
    });
    return result;
}

bool ChronoCache::persist(const std::string& key) {
    check_accepting_ops();

    bool ttl_removed = false;
    std::string current_value;
    hash_map.process_and_remove_if(key, [&](CacheEntry* e) {
        if (!e) return false;
        if (e->is_expired()) return true;
        if (e->has_ttl()) {
            e->remove_ttl();
            current_value = e->get_value();
            ttl_removed = true;
            if (check_logging_allowed()) {
                cache_event_logger->log_persist(key, current_value);
            }
        }
        return false;
    });

    return ttl_removed;
}

bool ChronoCache::zadd(const std::string& key, double score, const std::string& member) {
    check_accepting_ops();

    return sorted_sets.zadd(key, score, member);
}

std::optional<double> ChronoCache::zscore(const std::string& key, const std::string& member) const {
    (const_cast<ChronoCache*>(this))->check_accepting_ops();

    return sorted_sets.zscore(key, member);
}

bool ChronoCache::zrem(const std::string& key, const std::string& member) {
    check_accepting_ops();

    return sorted_sets.zrem(key, member);
}

std::optional<int> ChronoCache::zrank(const std::string& key, const std::string& member) const {
    (const_cast<ChronoCache*>(this))->check_accepting_ops();

    return sorted_sets.zrank(key, member);
}


void ChronoCache::replay() {
    CacheEventConsumer consumer(kafka_brokers, kafka_topic);
    std::vector<CacheEvent> events = consumer.consume_all_events();

    std::unordered_map<std::string, uint64_t> last_applied_seq;

    int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    for (CacheEvent& event : events) {
        const std::string& key   = event.key;
        const std::string& value = event.value;
        int64_t  ttl_ms = event.ttl_ms;
        uint64_t seq    = event.seq;

        if (last_applied_seq[key] >= seq) {
            continue;
        }
        last_applied_seq[key] = seq;

        switch (event.type) {
            case EventType::SET: {
                if (ttl_ms > 0) {
                    int64_t expires_at_ms = event.timestamp + ttl_ms;
                    if (expires_at_ms <= now_ms) break; // expired during downtime 
                    int64_t remaining_ms = expires_at_ms - now_ms;
                    this->set(key, value, std::chrono::milliseconds(remaining_ms));
                } else {
                    this->set(key, value, std::nullopt);
                }
                break;
            }

            case EventType::DELETE: {
                this->del(key);
                break;
            }

            case EventType::EXPIRE: {
                int64_t expires_at_ms = event.timestamp + ttl_ms;
                if (expires_at_ms <= now_ms) break; // expired during downtime 
                int64_t remaining_ms = expires_at_ms - now_ms;
                this->set(key, value, std::chrono::milliseconds(remaining_ms));
                break;
            }

            case EventType::PERSIST: {
                this->set(key, value, std::nullopt);
                break;
            }

            default: {
                throw std::logic_error("Unknown event type");
            }
        }
    }

    if (cache_event_logger) {
        cache_event_logger->set_seq_counters(last_applied_seq);
    }
}

bool ChronoCache::init(bool with_replay) {
    if (state == ChronoCacheState::READY) {
        return true;
    } 

    if (state != ChronoCacheState::UNINITIALIZED) {
        throw std::runtime_error("ChronoCache::init() requires UNINITIALIZED state");
    }

    if (!disable_event_logging) {
        cache_event_logger.emplace(kafka_brokers, kafka_topic);
    }

    if (!with_replay) {
        state = ChronoCacheState::READY;
        return true;
    }

    state = ChronoCacheState::REPLAYING;
    try {
        replay();
        state = ChronoCacheState::REPLAY_SUCCESS;
    } catch (const std::exception&) {
        state = ChronoCacheState::REPLAY_FAILED;
        throw;
    }

    if(state == ChronoCacheState::REPLAY_SUCCESS) {
        state = ChronoCacheState::READY;
        return true;
    } else {
        throw std::runtime_error("Replay failed");
        return false;
    }
}