#include <iostream>
#include <string>

#include "store/hash_map.h"

int main() {
    std::cout << "chrono-cache" << std::endl;

    ChronCacheHashMap<std::string, std::string> cache(16);
    cache.set("hello", "world");
    std::cout << "hello -> " << cache.get("hello") << std::endl;

    return 0;
}
