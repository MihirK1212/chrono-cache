#ifndef CHRONO_CACHE_SRC_CPP_CACHE_ENTRY_H
#define CHRONO_CACHE_SRC_CPP_CACHE_ENTRY_H

#include <chrono>
#include <optional>
#include <string>

// custom chrono cache entry that wraps the value with the ttl mechanism
struct CacheEntry {
    using Clock = std::chrono::system_clock;
    using TimePoint = Clock::time_point;

    std::string value;
    std::optional<TimePoint> expires_at;

    CacheEntry() : value(), expires_at(std::nullopt) {}

    explicit CacheEntry(const std::string& val)
        : value(val), expires_at(std::nullopt) {}

    CacheEntry(const std::string& val, std::chrono::milliseconds ttl)
        : value(val), expires_at(Clock::now() + ttl) {}

    bool is_expired() const {
        if (!expires_at) return false;
        return Clock::now() > *expires_at;
    }

    bool has_ttl() const {
        return expires_at.has_value();
    }

    std::optional<std::chrono::milliseconds> remaining_ttl() const {
        if (!expires_at) return std::nullopt;
        auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            *expires_at - Clock::now()
        );
        return remaining;
    }

    const std::string& get_value() const {
        return value;
    }

    void update_ttl(std::chrono::milliseconds ttl) {
        expires_at = Clock::now() + ttl;
    }

    void remove_ttl() {
        expires_at = std::nullopt;
    }
};

#endif
