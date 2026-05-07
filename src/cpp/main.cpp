#include <iostream>
#include <string>

#include "kv_store/hash_map.h"
#include "sorted_sets/sorted_set.h"

int main() {
    std::cout << "chrono-cache" << std::endl;

    ChronCacheHashMap<std::string, std::string> cache(16);
    cache.set("hello", "world");
    auto cached_value = cache.get("hello");
    if (cached_value.has_value()) {
        std::cout << "hello -> " << cached_value.value() << std::endl;
    }

    SortedSetsAPI sorted_sets;
    sorted_sets.zadd("leaderboard", 1.0, "hello");
    sorted_sets.zadd("leaderboard", 2.0, "world");

    auto hello_score = sorted_sets.zscore("leaderboard", "hello");
    auto world_rank = sorted_sets.zrank("leaderboard", "world");
    if (hello_score.has_value()) {
        std::cout << "leaderboard hello score -> " << hello_score.value() << std::endl;
    }
    if (world_rank.has_value()) {
        std::cout << "leaderboard world rank -> " << world_rank.value() << std::endl;
    }

    sorted_sets.zrem("leaderboard", "hello");
    auto hello_score_after_removal = sorted_sets.zscore("leaderboard", "hello");
    if (hello_score_after_removal.has_value()) {
        std::cout << "leaderboard hello score -> " << hello_score_after_removal.value() << std::endl;
    }
    auto world_rank_after_removal = sorted_sets.zrank("leaderboard", "world");
    if (world_rank_after_removal.has_value()) {
        std::cout << "leaderboard world rank -> " << world_rank_after_removal.value() << std::endl;
    }

    return 0;
}
