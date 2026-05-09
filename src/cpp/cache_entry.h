#ifndef CHRONO_CACHE_SRC_CPP_CACHE_ENTRY_H
#define CHRONO_CACHE_SRC_CPP_CACHE_ENTRY_H

#include <chrono>
#include <optional>
#include <string>

struct CacheEntry {
    using Clock = std::chrono::steady_clock;
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

    // Returns remaining TTL, or nullopt if this entry has no TTL set.
    // Callers should check is_expired() before calling this.
    std::optional<std::chrono::milliseconds> remaining_ttl() const {
        if (!expires_at) return std::nullopt;
        auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            *expires_at - Clock::now()
        );
        return remaining;
    }
};

#endif
