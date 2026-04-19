#ifndef CHRONO_CACHE_SRC_CPP_STORE_HASH_KEY_H
#define CHRONO_CACHE_SRC_CPP_STORE_HASH_KEY_H

#include <string>
#include <stdexcept>

template<typename T>

class ChronCacheHashKey {
    private:

    T raw_key;
    std::string hashed_key;

    std::string hash_function(const T& key) const {
        // allow only numerical and string keys
        if(
            !(
                std::is_same<T, int>::value 
                || std::is_same<T, std::string>::value
                || std::is_same<T, float>::value 
                || std::is_same<T, double>::value
                || std::is_same<T, long>::value
                || std::is_same<T, long long>::value
                || std::is_same<T, unsigned int>::value
                || std::is_same<T, unsigned long>::value
                || std::is_same<T, unsigned long long>::value
                || std::is_same<T, char>::value
                || std::is_same<T, unsigned char>::value
                || std::is_same<T, short>::value
                || std::is_same<T, unsigned short>::value
                || std::is_same<T, bool>::value
                || std::is_same<T, long double>::value
                || std::is_same<T, wchar_t>::value
            )
        ) {
            throw std::runtime_error("Invalid key type");
        }
        return std::hash<std::string>()(key);
    }

    public:
    ChronCacheHashKey(const T& key) {
        raw_key = key;
        hashed_key = hash_function(key);
    }
    
    ~ChronCacheHashKey() = default;

    const std::string& get() const {
        return hashed_key;
    }

    const T& get_raw() const {
        return raw_key;
    }

    // purposley delete the copy and move constructors and operators
    // this prevents double free errors
    ChronCacheHashKey(const ChronCacheHashKey&) = delete;
    ChronCacheHashKey& operator=(const ChronCacheHashKey&) = delete;
    ChronCacheHashKey(ChronCacheHashKey&&) = delete;
    ChronCacheHashKey& operator=(ChronCacheHashKey&&) = delete;
};

#endif