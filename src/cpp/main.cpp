#include <iostream>
#include <string>

#include "chrono_cache.h"

int main() {
    ChronoCache cache;

    // --- Key-Value Store Operations ---
    std::cout << "=== Key-Value Store ===" << std::endl;

    cache.set("name", "chrono-cache");
    cache.set("version", "1.0.0");
    cache.set("language", "C++");

    auto name = cache.get("name");
    auto version = cache.get("version");
    auto language = cache.get("language");
    std::cout << "name    -> " << name.value_or("(nil)") << std::endl;
    std::cout << "version -> " << version.value_or("(nil)") << std::endl;
    std::cout << "language -> " << language.value_or("(nil)") << std::endl;

    cache.set("version", "2.0.0");
    std::cout << "version (after update) -> " << cache.get("version").value_or("(nil)") << std::endl;

    cache.del("language");
    std::cout << "language (after del)   -> " << cache.get("language").value_or("(nil)") << std::endl;

    auto missing = cache.get("nonexistent");
    std::cout << "nonexistent key        -> " << missing.value_or("(nil)") << std::endl;

    // --- Sorted Set Operations ---
    std::cout << "\n=== Sorted Sets ===" << std::endl;

    cache.zadd("leaderboard", 1500.0, "alice");
    cache.zadd("leaderboard", 2300.0, "bob");
    cache.zadd("leaderboard", 1800.0, "charlie");
    cache.zadd("leaderboard", 2100.0, "diana");

    std::cout << "alice   score -> " << cache.zscore("leaderboard", "alice").value() << std::endl;
    std::cout << "bob     score -> " << cache.zscore("leaderboard", "bob").value() << std::endl;
    std::cout << "charlie score -> " << cache.zscore("leaderboard", "charlie").value() << std::endl;
    std::cout << "diana   score -> " << cache.zscore("leaderboard", "diana").value() << std::endl;

    std::cout << "alice   rank -> " << cache.zrank("leaderboard", "alice").value() << std::endl;
    std::cout << "bob     rank -> " << cache.zrank("leaderboard", "bob").value() << std::endl;
    std::cout << "charlie rank -> " << cache.zrank("leaderboard", "charlie").value() << std::endl;
    std::cout << "diana   rank -> " << cache.zrank("leaderboard", "diana").value() << std::endl;

    cache.zadd("leaderboard", 2500.0, "alice");
    std::cout << "\nalice score (after update) -> " << cache.zscore("leaderboard", "alice").value() << std::endl;
    std::cout << "alice rank  (after update) -> " << cache.zrank("leaderboard", "alice").value() << std::endl;

    cache.zrem("leaderboard", "charlie");
    auto charlie_score = cache.zscore("leaderboard", "charlie");
    std::cout << "\ncharlie score (after zrem) -> " << (charlie_score.has_value() ? std::to_string(charlie_score.value()) : "(nil)") << std::endl;
    std::cout << "bob     rank  (after zrem) -> " << cache.zrank("leaderboard", "bob").value() << std::endl;
    std::cout << "diana   rank  (after zrem) -> " << cache.zrank("leaderboard", "diana").value() << std::endl;

    return 0;
}
