#ifndef CACHE_EVENT_H
#define CACHE_EVENT_H

#include <stdexcept>
#include <string>
#include <cstdint>

enum class EventType { SET, DELETE, EXPIRE };

inline std::string event_type_name(EventType t) {
    switch (t) {
        case EventType::SET:    return "SET";
        case EventType::DELETE: return "DELETE";
        case EventType::EXPIRE: return "EXPIRE";
    }
    return "UNKNOWN";
}

inline EventType event_type_from_string(const std::string& s) {
    if (s == "SET")    return EventType::SET;
    if (s == "DELETE") return EventType::DELETE;
    if (s == "EXPIRE") return EventType::EXPIRE;
    throw std::invalid_argument("Unknown EventType: " + s);
}

struct CacheEvent {
    EventType   type      = EventType::SET;
    std::string key;
    std::string value;
    int64_t     ttl_ms    = 0;
    uint64_t    seq       = 0;
    int64_t     timestamp = 0;
};

#endif
