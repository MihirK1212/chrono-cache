#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "chrono_cache.h"

static void print_pttl(ChronoCache& cache, const std::string& key) {
    long long ms = cache.pttl(key);
    if (ms == -2) std::cout << "pttl(" << key << ") -> (nil) [key not found]\n";
    else if (ms == -1) std::cout << "pttl(" << key << ") -> -1 [no TTL / persistent]\n";
    else std::cout << "pttl(" << key << ") -> " << ms << " ms remaining\n";
}

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

    // --- TTL Operations ---
    std::cout << "\n=== TTL (Time-To-Live) ===" << std::endl;

    // Basic TTL: key expires after 200 ms
    cache.set("token", "abc123", std::chrono::milliseconds(200));
    std::cout << "token (before expiry) -> " << cache.get("token").value_or("(nil)") << "\n";
    print_pttl(cache, "token");

    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    std::cout << "token (after 250ms)   -> " << cache.get("token").value_or("(nil)") << "\n";
    print_pttl(cache, "token");

    // pttl on a persistent key returns -1
    std::cout << "\n--- persistent key ---\n";
    cache.set("persistent", "stays forever");
    print_pttl(cache, "persistent");

    // pttl on a nonexistent key returns -2
    std::cout << "\n--- nonexistent key ---\n";
    print_pttl(cache, "ghost");

    // expire: attach a TTL to an already-existing key
    std::cout << "\n--- expire() on existing key ---\n";
    cache.set("session", "user42");
    cache.expire("session", std::chrono::milliseconds(300));
    std::cout << "session (before expiry) -> " << cache.get("session").value_or("(nil)") << "\n";
    print_pttl(cache, "session");

    std::this_thread::sleep_for(std::chrono::milliseconds(350));

    std::cout << "session (after 350ms)   -> " << cache.get("session").value_or("(nil)") << "\n";
    print_pttl(cache, "session");

    // persist: remove TTL from a key mid-flight
    std::cout << "\n--- persist() removes TTL ---\n";
    cache.set("temp", "i might live", std::chrono::milliseconds(500));
    print_pttl(cache, "temp");
    cache.persist("temp");
    print_pttl(cache, "temp");

    std::this_thread::sleep_for(std::chrono::milliseconds(550));

    std::cout << "temp (after 550ms, TTL was removed) -> " << cache.get("temp").value_or("(nil)") << "\n";

    // set with TTL, then overwrite with no TTL — TTL is cleared
    std::cout << "\n--- overwrite clears TTL ---\n";
    cache.set("overwrite_me", "v1", std::chrono::milliseconds(200));
    print_pttl(cache, "overwrite_me");
    cache.set("overwrite_me", "v2");  // no TTL — resets expiry
    print_pttl(cache, "overwrite_me");
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    std::cout << "overwrite_me (after 250ms) -> " << cache.get("overwrite_me").value_or("(nil)") << "\n";

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
