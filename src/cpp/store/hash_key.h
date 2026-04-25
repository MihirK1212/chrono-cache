#ifndef CHRONO_CACHE_SRC_CPP_STORE_HASH_KEY_H
#define CHRONO_CACHE_SRC_CPP_STORE_HASH_KEY_H

#include <string>
#include <type_traits>

template<typename T>
class ChronCacheHashKey {
    static_assert(
        std::is_arithmetic<T>::value || std::is_same<T, std::string>::value,
        "Key type must be arithmetic or std::string"
    );

    private:
    T raw_key;
    size_t hashed_key;
    public:
    ChronCacheHashKey(const T& key)
        : raw_key(key)
        , hashed_key(std::hash<T>()(key)) {}

    ~ChronCacheHashKey() = default;

    const size_t& get() const {
        return hashed_key;
    }

    const T& get_raw() const {
        return raw_key;
    }

    ChronCacheHashKey(const ChronCacheHashKey&) = delete;
    ChronCacheHashKey& operator=(const ChronCacheHashKey&) = delete;
    ChronCacheHashKey(ChronCacheHashKey&&) = delete;
    ChronCacheHashKey& operator=(ChronCacheHashKey&&) = delete;
};

#endif
