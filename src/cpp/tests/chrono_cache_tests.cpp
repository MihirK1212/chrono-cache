// =============================================================================
// chrono_cache_tests.cpp
//
// Exhaustive test suite for ChronoCache covering:
//   - All public API operations (set, get, del, expire, persist, pttl,
//     zadd, zscore, zrem, zrank)
//   - Normal, edge, and corner cases
//   - TTL / expiry semantics
//   - Concurrent access (race conditions)
//   - Stress tests that force hash-map resizes
// =============================================================================

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <numeric>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "../chrono_cache.h"

using ms = std::chrono::milliseconds;

// =============================================================================
// Minimal test framework
// =============================================================================

static int g_total   = 0;
static int g_passed  = 0;
static int g_failed  = 0;
static std::string g_suite;

#define ASSERT_TRUE(cond) do {                                                    \
    ++g_total;                                                                    \
    if (!(cond)) {                                                                \
        ++g_failed;                                                               \
        std::cerr << "  FAIL  [" << g_suite << "] " << __FILE__                  \
                  << ':' << __LINE__ << "  " << #cond << '\n';                   \
    } else { ++g_passed; }                                                        \
} while (0)

#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

#define ASSERT_EQ(a, b) do {                                                      \
    ++g_total;                                                                    \
    if (!((a) == (b))) {                                                          \
        ++g_failed;                                                               \
        std::cerr << "  FAIL  [" << g_suite << "] " << __FILE__                  \
                  << ':' << __LINE__ << "  (" << #a << ") == (" << #b << ")\n";  \
    } else { ++g_passed; }                                                        \
} while (0)

#define ASSERT_NE(a, b) do {                                                      \
    ++g_total;                                                                    \
    if ((a) == (b)) {                                                             \
        ++g_failed;                                                               \
        std::cerr << "  FAIL  [" << g_suite << "] " << __FILE__                  \
                  << ':' << __LINE__ << "  (" << #a << ") != (" << #b << ")\n";  \
    } else { ++g_passed; }                                                        \
} while (0)

static void run_suite(const std::string& name, std::function<void()> fn) {
    g_suite = name;
    std::cout << "[SUITE] " << name << '\n';
    fn();
}

static void print_results() {
    std::cout << "\n========================================\n";
    if (g_failed == 0) {
        std::cout << "ALL " << g_total << " TESTS PASSED\n";
    } else {
        std::cout << g_passed << '/' << g_total << " passed  —  "
                  << g_failed << " FAILED\n";
    }
    std::cout << "========================================\n";
}

// =============================================================================
// Helper utilities
// =============================================================================

static void sleep_ms(long long ms_val) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms_val));
}

// =============================================================================
// 1. SET / GET — basic correctness
// =============================================================================

static void test_set_get_basic() {
    run_suite("set/get — basic", [] {
        ChronoCache c;

        // Fresh key
        ASSERT_TRUE(c.set("foo", "bar"));
        auto v = c.get("foo");
        ASSERT_TRUE(v.has_value());
        ASSERT_EQ(*v, "bar");

        // Overwrite same key
        ASSERT_TRUE(c.set("foo", "baz"));
        v = c.get("foo");
        ASSERT_TRUE(v.has_value());
        ASSERT_EQ(*v, "baz");

        // Multiple distinct keys
        c.set("k1", "v1");
        c.set("k2", "v2");
        c.set("k3", "v3");
        ASSERT_EQ(*c.get("k1"), "v1");
        ASSERT_EQ(*c.get("k2"), "v2");
        ASSERT_EQ(*c.get("k3"), "v3");

        // Get non-existent key
        ASSERT_FALSE(c.get("nonexistent").has_value());

        // Empty key
        ASSERT_TRUE(c.set("", "empty_key_value"));
        ASSERT_EQ(*c.get(""), "empty_key_value");

        // Empty value
        ASSERT_TRUE(c.set("empty_val", ""));
        ASSERT_EQ(*c.get("empty_val"), "");

        // Both empty
        ASSERT_TRUE(c.set("", ""));
        ASSERT_EQ(*c.get(""), "");

        // Key with special characters
        std::string special_key = "key\0with\nnewlines\ttabs\x01\x02\xff";
        special_key[3] = '\0'; // ensure embedded null
        std::string special_val = "val\x00\xff\xfe";
        special_val[3] = '\0';
        ASSERT_TRUE(c.set(special_key, special_val));
        auto sv = c.get(special_key);
        ASSERT_TRUE(sv.has_value());
        ASSERT_EQ(*sv, special_val);

        // Very long key and value
        std::string long_key(10000, 'k');
        std::string long_val(100000, 'v');
        ASSERT_TRUE(c.set(long_key, long_val));
        ASSERT_EQ(*c.get(long_key), long_val);

        // Unicode-ish multi-byte strings
        std::string utf8_key = "\xe2\x98\x83 snowman";
        std::string utf8_val = "\xf0\x9f\x98\x80 emoji";
        ASSERT_TRUE(c.set(utf8_key, utf8_val));
        ASSERT_EQ(*c.get(utf8_key), utf8_val);

        // Overwrite: no TTL → has TTL
        c.set("switch_ttl", "original");
        c.set("switch_ttl", "with_ttl", ms(5000));
        ASSERT_EQ(*c.get("switch_ttl"), "with_ttl");
        ASSERT_TRUE(c.pttl("switch_ttl") > 0);

        // Overwrite: has TTL → no TTL
        c.set("remove_ttl", "val", ms(5000));
        c.set("remove_ttl", "no_ttl_now");
        ASSERT_EQ(*c.get("remove_ttl"), "no_ttl_now");
        // After overwrite without TTL, new entry has no TTL
        ASSERT_EQ(c.pttl("remove_ttl"), -1LL);
    });
}

// =============================================================================
// 2. SET with TTL / expiry
// =============================================================================

static void test_set_get_ttl() {
    run_suite("set/get — TTL & expiry", [] {
        ChronoCache c;

        // Key is immediately visible when TTL > elapsed
        c.set("alive", "yes", ms(5000));
        ASSERT_EQ(*c.get("alive"), "yes");

        // Key disappears after TTL elapses
        c.set("short", "bye", ms(50));
        ASSERT_TRUE(c.get("short").has_value());
        sleep_ms(100);
        ASSERT_FALSE(c.get("short").has_value());

        // Very short TTL (1 ms) — expect expiry quickly
        c.set("veryshort", "x", ms(1));
        sleep_ms(50);
        ASSERT_FALSE(c.get("veryshort").has_value());

        // TTL of 0ms — should expire immediately or on first access
        c.set("zeroTTL", "z", ms(0));
        sleep_ms(10);
        ASSERT_FALSE(c.get("zeroTTL").has_value());

        // After expiry, setting the same key works as a fresh insert
        c.set("reborn", "first", ms(50));
        sleep_ms(100);
        ASSERT_FALSE(c.get("reborn").has_value());
        c.set("reborn", "second");
        ASSERT_EQ(*c.get("reborn"), "second");

        // Key without TTL never expires
        c.set("immortal", "forever");
        sleep_ms(20);
        ASSERT_EQ(*c.get("immortal"), "forever");
    });
}

// =============================================================================
// 3. DEL
// =============================================================================

static void test_del() {
    run_suite("del", [] {
        ChronoCache c;

        // Delete existing key
        c.set("d1", "val");
        ASSERT_TRUE(c.del("d1"));
        ASSERT_FALSE(c.get("d1").has_value());

        // Delete non-existent key
        ASSERT_FALSE(c.del("ghost"));

        // Delete already-expired key: should behave as missing
        c.set("expired_del", "v", ms(50));
        sleep_ms(100);
        // The key is logically expired; del still tries to remove the node
        // The underlying remove does a raw bucket scan — it may find the stale node.
        // We just verify we can call del without crashing; the value is gone.
        c.del("expired_del");
        ASSERT_FALSE(c.get("expired_del").has_value());

        // Delete key, then re-insert
        c.set("phoenix", "old");
        c.del("phoenix");
        c.set("phoenix", "new");
        ASSERT_EQ(*c.get("phoenix"), "new");

        // Delete the same key twice
        c.set("twice", "v");
        ASSERT_TRUE(c.del("twice"));
        ASSERT_FALSE(c.del("twice"));

        // Delete empty-string key
        c.set("", "empty");
        ASSERT_TRUE(c.del(""));
        ASSERT_FALSE(c.get("").has_value());
    });
}

// =============================================================================
// 4. EXPIRE
// =============================================================================

static void test_expire() {
    run_suite("expire", [] {
        ChronoCache c;

        // Setting TTL on an existing, non-expired key returns true
        c.set("e1", "val");
        ASSERT_TRUE(c.expire("e1", ms(5000)));
        ASSERT_TRUE(c.pttl("e1") > 0);

        // Key actually expires after the new TTL
        c.set("e2", "val");
        ASSERT_TRUE(c.expire("e2", ms(50)));
        sleep_ms(100);
        ASSERT_FALSE(c.get("e2").has_value());

        // expire on non-existent key returns false
        ASSERT_FALSE(c.expire("ghost", ms(1000)));

        // expire on an already-expired key returns false (treated as missing)
        c.set("e3", "val", ms(50));
        sleep_ms(100);
        ASSERT_FALSE(c.expire("e3", ms(5000)));

        // Extending an existing TTL
        c.set("e4", "val", ms(50));
        ASSERT_TRUE(c.expire("e4", ms(5000)));  // extend before expiry
        sleep_ms(100);
        ASSERT_TRUE(c.get("e4").has_value()); // should still be alive

        // Shortening an existing TTL
        c.set("e5", "val", ms(5000));
        ASSERT_TRUE(c.expire("e5", ms(50)));
        sleep_ms(100);
        ASSERT_FALSE(c.get("e5").has_value());
    });
}

// =============================================================================
// 5. PERSIST
// =============================================================================

static void test_persist() {
    run_suite("persist", [] {
        ChronoCache c;

        // persist removes TTL — key survives after original TTL would have elapsed
        c.set("p1", "val", ms(100));
        ASSERT_TRUE(c.persist("p1"));
        ASSERT_EQ(c.pttl("p1"), -1LL);
        sleep_ms(150);
        ASSERT_TRUE(c.get("p1").has_value());

        // persist on a key with no TTL is a no-op but still returns true
        // (the key exists and is not expired — implementation sets expires_at = nullopt)
        c.set("p2", "val");
        ASSERT_TRUE(c.persist("p2"));
        ASSERT_EQ(c.pttl("p2"), -1LL);

        // persist on non-existent key returns false
        ASSERT_FALSE(c.persist("ghost"));

        // persist on already-expired key returns false
        c.set("p3", "val", ms(50));
        sleep_ms(100);
        ASSERT_FALSE(c.persist("p3"));

        // persist then re-expire
        c.set("p4", "val", ms(5000));
        c.persist("p4");
        ASSERT_EQ(c.pttl("p4"), -1LL);
        c.expire("p4", ms(50));
        sleep_ms(100);
        ASSERT_FALSE(c.get("p4").has_value());
    });
}

// =============================================================================
// 6. PTTL
// =============================================================================

static void test_pttl() {
    run_suite("pttl", [] {
        ChronoCache c;

        // Non-existent key → -2
        ASSERT_EQ(c.pttl("missing"), -2LL);

        // Existing key with no TTL → -1
        c.set("notimely", "v");
        ASSERT_EQ(c.pttl("notimely"), -1LL);

        // Existing key with TTL → positive and ≤ requested TTL
        c.set("timely", "v", ms(5000));
        long long t = c.pttl("timely");
        ASSERT_TRUE(t > 0 && t <= 5000);

        // After expiry → -2
        c.set("dying", "v", ms(50));
        sleep_ms(100);
        ASSERT_EQ(c.pttl("dying"), -2LL);

        // After persist → -1
        c.set("persisted", "v", ms(5000));
        c.persist("persisted");
        ASSERT_EQ(c.pttl("persisted"), -1LL);

        // PTTL decreases over time (approx)
        c.set("tick", "v", ms(2000));
        long long t1 = c.pttl("tick");
        sleep_ms(50);
        long long t2 = c.pttl("tick");
        ASSERT_TRUE(t2 < t1);

        // After deletion → -2
        c.set("deleted", "v");
        c.del("deleted");
        ASSERT_EQ(c.pttl("deleted"), -2LL);
    });
}

// =============================================================================
// 7. ZADD / ZSCORE / ZREM / ZRANK — basic correctness
// =============================================================================

static void test_sorted_set_basic() {
    run_suite("zadd/zscore/zrem/zrank — basic", [] {
        ChronoCache c;

        // zadd returns true for new member
        ASSERT_TRUE(c.zadd("z", 1.0, "a"));

        // zscore returns correct score
        auto s = c.zscore("z", "a");
        ASSERT_TRUE(s.has_value());
        ASSERT_EQ(*s, 1.0);

        // zadd same score → returns false (no-op)
        ASSERT_FALSE(c.zadd("z", 1.0, "a"));

        // zadd different score → returns false (update)
        ASSERT_FALSE(c.zadd("z", 99.0, "a"));
        ASSERT_EQ(*c.zscore("z", "a"), 99.0);

        // zscore non-existent member
        ASSERT_FALSE(c.zscore("z", "nonexistent").has_value());

        // zscore non-existent set
        ASSERT_FALSE(c.zscore("noset", "m").has_value());

        // zrem existing member → true
        c.zadd("z", 2.0, "b");
        ASSERT_TRUE(c.zrem("z", "b"));
        ASSERT_FALSE(c.zscore("z", "b").has_value());

        // zrem non-existent member → false
        ASSERT_FALSE(c.zrem("z", "ghost"));

        // zrem from non-existent set → false
        ASSERT_FALSE(c.zrem("noset", "m"));

        // zrank non-existent set → nullopt
        ASSERT_FALSE(c.zrank("noset", "m").has_value());

        // zrank non-existent member → nullopt
        ASSERT_FALSE(c.zrank("z", "ghost").has_value());

        // Multiple members — rank ordering by score ASC
        ChronoCache c2;
        c2.zadd("s", 1.0, "one");
        c2.zadd("s", 2.0, "two");
        c2.zadd("s", 3.0, "three");
        ASSERT_EQ(*c2.zrank("s", "one"),   0);
        ASSERT_EQ(*c2.zrank("s", "two"),   1);
        ASSERT_EQ(*c2.zrank("s", "three"), 2);

        // Rank after removal
        c2.zrem("s", "two");
        ASSERT_EQ(*c2.zrank("s", "one"),   0);
        ASSERT_EQ(*c2.zrank("s", "three"), 1);
    });
}

// =============================================================================
// 8. ZADD / ZRANK — score ordering & lexicographic tie-breaking
// =============================================================================

static void test_sorted_set_ordering() {
    run_suite("zadd/zrank — ordering", [] {
        ChronoCache c;

        // Identical scores — tiebreak by member name lexicographically
        c.zadd("lex", 5.0, "charlie");
        c.zadd("lex", 5.0, "alice");
        c.zadd("lex", 5.0, "bob");

        // Expected order: alice(0), bob(1), charlie(2)
        ASSERT_EQ(*c.zrank("lex", "alice"),   0);
        ASSERT_EQ(*c.zrank("lex", "bob"),     1);
        ASSERT_EQ(*c.zrank("lex", "charlie"), 2);

        // Mixed scores and names
        ChronoCache c2;
        c2.zadd("m", 3.0, "aardvark");
        c2.zadd("m", 1.0, "zebra");
        c2.zadd("m", 2.0, "mango");
        c2.zadd("m", 1.0, "apple"); // same score as zebra, lexically before

        // Expected ranks: apple(0, score=1), zebra(1, score=1), mango(2, score=2), aardvark(3, score=3)
        ASSERT_EQ(*c2.zrank("m", "apple"),    0);
        ASSERT_EQ(*c2.zrank("m", "zebra"),    1);
        ASSERT_EQ(*c2.zrank("m", "mango"),    2);
        ASSERT_EQ(*c2.zrank("m", "aardvark"), 3);

        // Negative scores
        ChronoCache c3;
        c3.zadd("neg", -10.0, "a");
        c3.zadd("neg", -5.0,  "b");
        c3.zadd("neg", 0.0,   "c");
        c3.zadd("neg", 5.0,   "d");
        ASSERT_EQ(*c3.zrank("neg", "a"), 0);
        ASSERT_EQ(*c3.zrank("neg", "b"), 1);
        ASSERT_EQ(*c3.zrank("neg", "c"), 2);
        ASSERT_EQ(*c3.zrank("neg", "d"), 3);

        // Large score values
        ChronoCache c4;
        c4.zadd("big", 1e15,  "x");
        c4.zadd("big", -1e15, "y");
        ASSERT_EQ(*c4.zrank("big", "y"), 0);
        ASSERT_EQ(*c4.zrank("big", "x"), 1);

        // Score update changes rank
        ChronoCache c5;
        c5.zadd("upd", 1.0, "low");
        c5.zadd("upd", 2.0, "mid");
        c5.zadd("upd", 3.0, "high");
        c5.zadd("upd", 0.0, "low"); // move "low" to score 0 — now rank 0
        ASSERT_EQ(*c5.zrank("upd", "low"), 0);
        ASSERT_EQ(*c5.zrank("upd", "mid"), 1);
        ASSERT_EQ(*c5.zrank("upd", "high"), 2);
    });
}

// =============================================================================
// 9. ZADD — multiple disjoint sets
// =============================================================================

static void test_sorted_set_multiple_sets() {
    run_suite("zadd — multiple disjoint sets", [] {
        ChronoCache c;

        c.zadd("setA", 1.0, "m1");
        c.zadd("setA", 2.0, "m2");
        c.zadd("setB", 10.0, "m1");
        c.zadd("setB", 20.0, "m3");

        // Same member name, different sets
        ASSERT_EQ(*c.zscore("setA", "m1"), 1.0);
        ASSERT_EQ(*c.zscore("setB", "m1"), 10.0);

        // m3 not in setA
        ASSERT_FALSE(c.zscore("setA", "m3").has_value());

        // Removing from one set doesn't affect another
        c.zrem("setA", "m1");
        ASSERT_FALSE(c.zscore("setA", "m1").has_value());
        ASSERT_EQ(*c.zscore("setB", "m1"), 10.0);

        // Rank independence
        ASSERT_EQ(*c.zrank("setB", "m1"), 0);
        ASSERT_EQ(*c.zrank("setB", "m3"), 1);
    });
}

// =============================================================================
// 10. ZADD — edge cases
// =============================================================================

static void test_sorted_set_edge_cases() {
    run_suite("zadd — edge cases", [] {
        ChronoCache c;

        // Empty string member
        ASSERT_TRUE(c.zadd("ez", 1.0, ""));
        ASSERT_EQ(*c.zscore("ez", ""), 1.0);
        ASSERT_EQ(*c.zrank("ez", ""), 0);

        // Empty string key (set name)
        ASSERT_TRUE(c.zadd("", 1.0, "member"));
        ASSERT_EQ(*c.zscore("", "member"), 1.0);

        // Very long member name
        std::string long_member(5000, 'm');
        ASSERT_TRUE(c.zadd("big_set", 42.0, long_member));
        ASSERT_EQ(*c.zscore("big_set", long_member), 42.0);

        // NaN / infinity scores — C++ allows these in double
        double inf = std::numeric_limits<double>::infinity();
        ASSERT_TRUE(c.zadd("inf_set", inf, "pos_inf"));
        ASSERT_TRUE(c.zadd("inf_set", -inf, "neg_inf"));
        ASSERT_EQ(*c.zrank("inf_set", "neg_inf"), 0);
        ASSERT_EQ(*c.zrank("inf_set", "pos_inf"), 1);

        // re-add after zrem with same score should return true
        c.zadd("reuse", 7.0, "x");
        c.zrem("reuse", "x");
        ASSERT_TRUE(c.zadd("reuse", 7.0, "x"));
    });
}

// =============================================================================
// 11. Interaction between KV operations and TTL
// =============================================================================

static void test_ttl_interactions() {
    run_suite("TTL interactions", [] {
        ChronoCache c;

        // expire after persist: key should expire again
        c.set("x", "v");
        c.persist("x");
        c.expire("x", ms(50));
        sleep_ms(100);
        ASSERT_FALSE(c.get("x").has_value());

        // persist after expire: key survives
        c.set("y", "v", ms(5000));
        c.persist("y");
        ASSERT_EQ(c.pttl("y"), -1LL);
        sleep_ms(20);
        ASSERT_TRUE(c.get("y").has_value());

        // del kills key regardless of TTL
        c.set("z", "v", ms(5000));
        c.del("z");
        ASSERT_FALSE(c.get("z").has_value());
        ASSERT_EQ(c.pttl("z"), -2LL);

        // set overwrites stale TTL without inheriting it
        c.set("ov", "first", ms(50));
        sleep_ms(100); // let it expire
        c.set("ov", "second");
        ASSERT_EQ(*c.get("ov"), "second");
        ASSERT_EQ(c.pttl("ov"), -1LL);

        // Repeated expire calls — last one wins
        c.set("multi_exp", "v");
        c.expire("multi_exp", ms(5000));
        c.expire("multi_exp", ms(50));
        sleep_ms(100);
        ASSERT_FALSE(c.get("multi_exp").has_value());
    });
}

// =============================================================================
// 12. Many keys — trigger internal hash-map resize
// =============================================================================

static void test_large_insert() {
    run_suite("large insert / resize stress", [] {
        ChronoCache c;
        const int N = 10000;

        for (int i = 0; i < N; ++i) {
            std::string k = "key_" + std::to_string(i);
            std::string v = "val_" + std::to_string(i);
            ASSERT_TRUE(c.set(k, v));
        }

        int missing = 0;
        for (int i = 0; i < N; ++i) {
            std::string k = "key_" + std::to_string(i);
            std::string v = "val_" + std::to_string(i);
            auto r = c.get(k);
            if (!r.has_value() || *r != v) ++missing;
        }
        ASSERT_EQ(missing, 0);

        // Delete all
        for (int i = 0; i < N; ++i) {
            c.del("key_" + std::to_string(i));
        }
        int present = 0;
        for (int i = 0; i < N; ++i) {
            if (c.get("key_" + std::to_string(i)).has_value()) ++present;
        }
        ASSERT_EQ(present, 0);
    });
}

// =============================================================================
// 13. Sorted-set large insert / rank correctness
// =============================================================================

static void test_sorted_set_large() {
    run_suite("sorted-set large insert / rank correctness", [] {
        ChronoCache c;
        const int N = 500;

        // Insert members with sequential scores
        for (int i = 0; i < N; ++i) {
            std::string m = "m" + std::to_string(i);
            ASSERT_TRUE(c.zadd("big", static_cast<double>(i), m));
        }

        // Every rank must equal the score index
        int bad = 0;
        for (int i = 0; i < N; ++i) {
            std::string m = "m" + std::to_string(i);
            auto r = c.zrank("big", m);
            if (!r.has_value() || *r != i) ++bad;
        }
        ASSERT_EQ(bad, 0);

        // Remove all even-indexed members, verify remaining ranks
        for (int i = 0; i < N; i += 2) {
            c.zrem("big", "m" + std::to_string(i));
        }
        // Odd-indexed members remain: m1, m3, m5, ...
        // Their expected ranks after removal: m1→0, m3→1, m5→2, ...
        int rank_errors = 0;
        for (int i = 1, expected = 0; i < N; i += 2, ++expected) {
            auto r = c.zrank("big", "m" + std::to_string(i));
            if (!r.has_value() || *r != expected) ++rank_errors;
        }
        ASSERT_EQ(rank_errors, 0);
    });
}

// =============================================================================
// 14. Concurrent SET / GET on disjoint keys (no data race)
// =============================================================================

static void test_concurrent_disjoint_keys() {
    run_suite("concurrent — disjoint key sets", [] {
        ChronoCache c;
        const int THREADS = 8;
        const int OPS = 500;
        std::atomic<int> errors{0};

        auto worker = [&](int tid) {
            for (int i = 0; i < OPS; ++i) {
                std::string key = "t" + std::to_string(tid) + "_k" + std::to_string(i);
                std::string val = "v" + std::to_string(i);
                c.set(key, val);
                auto r = c.get(key);
                if (!r.has_value() || *r != val) ++errors;
            }
        };

        std::vector<std::thread> threads;
        threads.reserve(THREADS);
        for (int t = 0; t < THREADS; ++t)
            threads.emplace_back(worker, t);
        for (auto& th : threads) th.join();

        ASSERT_EQ(errors.load(), 0);
    });
}

// =============================================================================
// 15. Concurrent SET on the same key (last-write-wins, no crash)
// =============================================================================

static void test_concurrent_same_key() {
    run_suite("concurrent — same key contention", [] {
        ChronoCache c;
        const int THREADS = 16;
        const int OPS = 200;
        std::atomic<int> set_errors{0};

        auto writer = [&](int tid) {
            for (int i = 0; i < OPS; ++i) {
                std::string val = "t" + std::to_string(tid) + "_i" + std::to_string(i);
                if (!c.set("hotkey", val)) ++set_errors;
            }
        };

        std::vector<std::thread> threads;
        threads.reserve(THREADS);
        for (int t = 0; t < THREADS; ++t)
            threads.emplace_back(writer, t);
        for (auto& th : threads) th.join();

        ASSERT_EQ(set_errors.load(), 0);

        // After all writers finish, the key must exist with some value
        auto v = c.get("hotkey");
        ASSERT_TRUE(v.has_value());
    });
}

// =============================================================================
// 16. Concurrent SET + GET on the same key (readers never see corrupt data)
// =============================================================================

static void test_concurrent_set_get() {
    run_suite("concurrent — concurrent set+get, value integrity", [] {
        ChronoCache c;
        c.set("shared", "init");
        const int WRITERS = 4;
        const int READERS = 4;
        const int OPS = 300;
        std::atomic<bool> corrupt{false};

        // Every written value has a predictable prefix; readers verify this.
        auto writer = [&](int tid) {
            for (int i = 0; i < OPS; ++i) {
                c.set("shared", "val_" + std::to_string(tid) + "_" + std::to_string(i));
            }
        };

        auto reader = [&]() {
            for (int i = 0; i < OPS * WRITERS; ++i) {
                auto v = c.get("shared");
                if (v.has_value()) {
                    // Value must start with "val_" or be "init"
                    const std::string& s = *v;
                    if (s != "init" && s.substr(0, 4) != "val_") {
                        corrupt.store(true);
                    }
                }
            }
        };

        std::vector<std::thread> threads;
        for (int t = 0; t < WRITERS; ++t) threads.emplace_back(writer, t);
        for (int t = 0; t < READERS; ++t) threads.emplace_back(reader);
        for (auto& th : threads) th.join();

        ASSERT_FALSE(corrupt.load());
    });
}

// =============================================================================
// 17. Concurrent DEL + GET (no use-after-free, no crash)
// =============================================================================

static void test_concurrent_del_get() {
    run_suite("concurrent — del+get race", [] {
        ChronoCache c;
        const int THREADS = 8;
        const int OPS = 300;
        std::atomic<int> crashes{0};

        // pre-load keys
        for (int i = 0; i < OPS; ++i)
            c.set("dk_" + std::to_string(i), "val");

        std::atomic<int> idx{0};

        auto deleter = [&]() {
            int i;
            while ((i = idx.fetch_add(1)) < OPS) {
                c.del("dk_" + std::to_string(i));
            }
        };

        auto getter = [&]() {
            for (int i = 0; i < OPS; ++i) {
                try {
                    c.get("dk_" + std::to_string(i)); // just must not crash
                } catch (...) {
                    ++crashes;
                }
            }
        };

        std::vector<std::thread> threads;
        for (int t = 0; t < THREADS / 2; ++t) threads.emplace_back(deleter);
        for (int t = 0; t < THREADS / 2; ++t) threads.emplace_back(getter);
        for (auto& th : threads) th.join();

        ASSERT_EQ(crashes.load(), 0);
    });
}

// =============================================================================
// 18. Concurrent EXPIRE + GET (lazy eviction under contention)
// =============================================================================

static void test_concurrent_expire_get() {
    run_suite("concurrent — expire+get race", [] {
        ChronoCache c;
        const int N = 200;
        const int THREADS = 6;
        std::atomic<int> crashes{0};

        for (int i = 0; i < N; ++i)
            c.set("ek_" + std::to_string(i), "v");

        auto expirer = [&]() {
            for (int i = 0; i < N; ++i) {
                c.expire("ek_" + std::to_string(i), ms(20));
            }
        };

        auto getter = [&]() {
            for (int rep = 0; rep < 5; ++rep) {
                for (int i = 0; i < N; ++i) {
                    try {
                        c.get("ek_" + std::to_string(i));
                    } catch (...) {
                        ++crashes;
                    }
                }
                sleep_ms(10);
            }
        };

        std::vector<std::thread> threads;
        threads.emplace_back(expirer);
        for (int t = 0; t < THREADS; ++t) threads.emplace_back(getter);
        for (auto& th : threads) th.join();

        ASSERT_EQ(crashes.load(), 0);

        // After TTL elapses, all keys should be gone
        sleep_ms(50);
        int alive = 0;
        for (int i = 0; i < N; ++i) {
            if (c.get("ek_" + std::to_string(i)).has_value()) ++alive;
        }
        ASSERT_EQ(alive, 0);
    });
}

// =============================================================================
// 19. Concurrent ZADD + ZREM + ZSCORE on same set
// =============================================================================

static void test_concurrent_sorted_set() {
    run_suite("concurrent — zadd+zrem+zscore race", [] {
        ChronoCache c;
        const int THREADS = 8;
        const int OPS = 200;
        std::atomic<int> crashes{0};

        auto adder = [&](int tid) {
            for (int i = 0; i < OPS; ++i) {
                try {
                    c.zadd("cz", static_cast<double>(i), "m_t" + std::to_string(tid) + "_" + std::to_string(i));
                } catch (...) { ++crashes; }
            }
        };

        auto remover = [&](int tid) {
            for (int i = 0; i < OPS; ++i) {
                try {
                    c.zrem("cz", "m_t" + std::to_string(tid) + "_" + std::to_string(i));
                } catch (...) { ++crashes; }
            }
        };

        auto scorer = [&](int tid) {
            for (int i = 0; i < OPS; ++i) {
                try {
                    c.zscore("cz", "m_t" + std::to_string(tid) + "_" + std::to_string(i));
                } catch (...) { ++crashes; }
            }
        };

        std::vector<std::thread> threads;
        for (int t = 0; t < THREADS / 3 + 1; ++t) threads.emplace_back(adder, t);
        for (int t = 0; t < THREADS / 3 + 1; ++t) threads.emplace_back(remover, t);
        for (int t = 0; t < THREADS / 3 + 1; ++t) threads.emplace_back(scorer, t);
        for (auto& th : threads) th.join();

        ASSERT_EQ(crashes.load(), 0);
    });
}

// =============================================================================
// 20. Concurrent inserts that force multiple hash-map resizes
// =============================================================================

static void test_concurrent_resize_stress() {
    run_suite("concurrent — resize under load", [] {
        ChronoCache c;
        const int THREADS = 8;
        const int KEYS_PER_THREAD = 2000; // 16 000 total — well past initial cap
        std::atomic<int> errors{0};

        auto worker = [&](int tid) {
            for (int i = 0; i < KEYS_PER_THREAD; ++i) {
                std::string k = "r_" + std::to_string(tid) + "_" + std::to_string(i);
                c.set(k, std::to_string(i));
            }
            // Verify own keys after all sets
            for (int i = 0; i < KEYS_PER_THREAD; ++i) {
                std::string k = "r_" + std::to_string(tid) + "_" + std::to_string(i);
                auto v = c.get(k);
                if (!v.has_value() || *v != std::to_string(i)) ++errors;
            }
        };

        std::vector<std::thread> threads;
        threads.reserve(THREADS);
        for (int t = 0; t < THREADS; ++t) threads.emplace_back(worker, t);
        for (auto& th : threads) th.join();

        ASSERT_EQ(errors.load(), 0);
    });
}

// =============================================================================
// 21. Mixed concurrent KV + sorted-set operations
// =============================================================================

static void test_concurrent_mixed_ops() {
    run_suite("concurrent — mixed KV + sorted-set", [] {
        ChronoCache c;
        const int THREADS = 6;
        const int OPS = 300;
        std::atomic<int> crashes{0};

        auto kv_worker = [&](int tid) {
            for (int i = 0; i < OPS; ++i) {
                std::string k = "mk_" + std::to_string(tid) + "_" + std::to_string(i);
                try {
                    c.set(k, "v");
                    c.get(k);
                    c.expire(k, ms(500));
                    c.pttl(k);
                    c.persist(k);
                    c.del(k);
                } catch (...) { ++crashes; }
            }
        };

        auto ss_worker = [&](int tid) {
            for (int i = 0; i < OPS; ++i) {
                std::string m = "mm_" + std::to_string(tid) + "_" + std::to_string(i);
                try {
                    c.zadd("mix_set", static_cast<double>(i % 50), m);
                    c.zscore("mix_set", m);
                    c.zrank("mix_set", m);
                    if (i % 3 == 0) c.zrem("mix_set", m);
                } catch (...) { ++crashes; }
            }
        };

        std::vector<std::thread> threads;
        for (int t = 0; t < THREADS / 2; ++t) threads.emplace_back(kv_worker, t);
        for (int t = 0; t < THREADS / 2; ++t) threads.emplace_back(ss_worker, t);
        for (auto& th : threads) th.join();

        ASSERT_EQ(crashes.load(), 0);
    });
}

// =============================================================================
// 22. TTL expiry under heavy concurrent read pressure
// =============================================================================

static void test_ttl_concurrent_expiry() {
    run_suite("concurrent — TTL expiry under read pressure", [] {
        ChronoCache c;
        const int N = 500;
        const int READERS = 8;
        std::atomic<int> crashes{0};

        // Insert keys with short TTLs
        for (int i = 0; i < N; ++i)
            c.set("tce_" + std::to_string(i), "v", ms(30));

        // Readers hammering gets while keys expire
        auto reader = [&]() {
            for (int rep = 0; rep < 10; ++rep) {
                for (int i = 0; i < N; ++i) {
                    try { c.get("tce_" + std::to_string(i)); }
                    catch (...) { ++crashes; }
                }
                sleep_ms(5);
            }
        };

        std::vector<std::thread> threads;
        for (int t = 0; t < READERS; ++t) threads.emplace_back(reader);
        for (auto& th : threads) th.join();

        ASSERT_EQ(crashes.load(), 0);

        // After sufficient wait, all should be expired
        sleep_ms(100);
        int alive = 0;
        for (int i = 0; i < N; ++i)
            if (c.get("tce_" + std::to_string(i)).has_value()) ++alive;
        ASSERT_EQ(alive, 0);
    });
}

// =============================================================================
// 23. del on a key being concurrently written
// =============================================================================

static void test_concurrent_set_del() {
    run_suite("concurrent — set+del on same key", [] {
        ChronoCache c;
        const int ITERS = 1000;
        std::atomic<int> crashes{0};

        auto setter = [&]() {
            for (int i = 0; i < ITERS; ++i) {
                try { c.set("race", "v" + std::to_string(i)); }
                catch (...) { ++crashes; }
            }
        };
        auto deleter = [&]() {
            for (int i = 0; i < ITERS; ++i) {
                try { c.del("race"); }
                catch (...) { ++crashes; }
            }
        };
        auto reader = [&]() {
            for (int i = 0; i < ITERS; ++i) {
                try { c.get("race"); }
                catch (...) { ++crashes; }
            }
        };

        std::vector<std::thread> threads;
        threads.emplace_back(setter);
        threads.emplace_back(setter);
        threads.emplace_back(deleter);
        threads.emplace_back(reader);
        threads.emplace_back(reader);
        for (auto& th : threads) th.join();

        ASSERT_EQ(crashes.load(), 0);
    });
}

// =============================================================================
// 24. PTTL monotonically decreases (no negative TOCTOU glitch)
// =============================================================================

static void test_pttl_monotone() {
    run_suite("pttl — monotone decrease", [] {
        ChronoCache c;
        c.set("mono", "v", ms(2000));
        long long prev = c.pttl("mono");
        int inversions = 0;
        for (int i = 0; i < 20; ++i) {
            sleep_ms(5);
            long long curr = c.pttl("mono");
            if (curr > prev) ++inversions; // small clock jitter is tolerated — just flag
            prev = curr;
        }
        // Allow at most 1 inversion due to clock jitter
        ASSERT_TRUE(inversions <= 1);
    });
}

// =============================================================================
// 25. Expire + persist race (correctness, not crashing)
// =============================================================================

static void test_concurrent_expire_persist() {
    run_suite("concurrent — expire+persist race", [] {
        ChronoCache c;
        const int ITERS = 500;
        std::atomic<int> crashes{0};

        c.set("ep", "v");

        auto expirer = [&]() {
            for (int i = 0; i < ITERS; ++i) {
                try { c.expire("ep", ms(200)); }
                catch (...) { ++crashes; }
            }
        };
        auto persister = [&]() {
            for (int i = 0; i < ITERS; ++i) {
                try { c.persist("ep"); }
                catch (...) { ++crashes; }
            }
        };
        auto getter = [&]() {
            for (int i = 0; i < ITERS; ++i) {
                try { c.get("ep"); }
                catch (...) { ++crashes; }
            }
        };

        std::vector<std::thread> threads;
        threads.emplace_back(expirer);
        threads.emplace_back(persister);
        threads.emplace_back(getter);
        for (auto& th : threads) th.join();

        ASSERT_EQ(crashes.load(), 0);
    });
}

// =============================================================================
// 26. Sorted-set: score update preserves correct rank for all other members
// =============================================================================

static void test_sorted_set_score_update_rank() {
    run_suite("sorted-set — score update preserves ranks", [] {
        ChronoCache c;

        std::vector<std::string> members = {"a","b","c","d","e","f","g","h"};
        for (int i = 0; i < (int)members.size(); ++i)
            c.zadd("ss_upd", static_cast<double>(i), members[i]);

        // Move "d" (score=3) to score=10 — it should now be last
        c.zadd("ss_upd", 10.0, "d");

        // Expected order: a(0) b(1) c(2) e(3) f(4) g(5) h(6) d(7)
        std::vector<std::pair<std::string,int>> expected = {
            {"a",0},{"b",1},{"c",2},{"e",3},{"f",4},{"g",5},{"h",6},{"d",7}
        };
        for (auto& [m, r] : expected) {
            auto rank = c.zrank("ss_upd", m);
            ASSERT_TRUE(rank.has_value());
            ASSERT_EQ(*rank, r);
        }
    });
}

// =============================================================================
// 27. Boundary: single-element sorted set
// =============================================================================

static void test_sorted_set_single_element() {
    run_suite("sorted-set — single element", [] {
        ChronoCache c;
        c.zadd("s1", 42.0, "only");
        ASSERT_EQ(*c.zrank("s1", "only"), 0);
        ASSERT_EQ(*c.zscore("s1", "only"), 42.0);
        ASSERT_TRUE(c.zrem("s1", "only"));
        ASSERT_FALSE(c.zscore("s1", "only").has_value());
        ASSERT_FALSE(c.zrank("s1", "only").has_value());
    });
}

// =============================================================================
// 28. Stress: random mixed KV workload with many threads
// =============================================================================

static void test_stress_random_kv() {
    run_suite("stress — random mixed KV workload", [] {
        ChronoCache c;
        const int THREADS = 10;
        const int OPS = 1000;
        std::atomic<int> crashes{0};

        auto worker = [&](int tid) {
            std::mt19937 rng(static_cast<unsigned>(tid) * 1234567);
            std::uniform_int_distribution<int> key_dist(0, 99);
            std::uniform_int_distribution<int> op_dist(0, 5);

            for (int i = 0; i < OPS; ++i) {
                int k = key_dist(rng);
                std::string key = "s_" + std::to_string(k);
                try {
                    switch (op_dist(rng)) {
                        case 0: c.set(key, "v" + std::to_string(i)); break;
                        case 1: c.get(key); break;
                        case 2: c.del(key); break;
                        case 3: c.expire(key, ms(200)); break;
                        case 4: c.persist(key); break;
                        case 5: c.pttl(key); break;
                    }
                } catch (...) { ++crashes; }
            }
        };

        std::vector<std::thread> threads;
        threads.reserve(THREADS);
        for (int t = 0; t < THREADS; ++t) threads.emplace_back(worker, t);
        for (auto& th : threads) th.join();

        ASSERT_EQ(crashes.load(), 0);
    });
}

// =============================================================================
// 29. Stress: random mixed sorted-set workload with many threads
// =============================================================================

static void test_stress_random_sorted_set() {
    run_suite("stress — random mixed sorted-set workload", [] {
        ChronoCache c;
        const int THREADS = 8;
        const int OPS = 1000;
        std::atomic<int> crashes{0};

        auto worker = [&](int tid) {
            std::mt19937 rng(static_cast<unsigned>(tid) * 987654);
            std::uniform_int_distribution<int> set_dist(0, 4);
            std::uniform_int_distribution<int> mem_dist(0, 49);
            std::uniform_int_distribution<int> op_dist(0, 3);
            std::uniform_real_distribution<double> score_dist(-100.0, 100.0);

            for (int i = 0; i < OPS; ++i) {
                std::string key = "ss_" + std::to_string(set_dist(rng));
                std::string member = "m" + std::to_string(mem_dist(rng));
                double score = score_dist(rng);
                try {
                    switch (op_dist(rng)) {
                        case 0: c.zadd(key, score, member); break;
                        case 1: c.zscore(key, member); break;
                        case 2: c.zrem(key, member); break;
                        case 3: c.zrank(key, member); break;
                    }
                } catch (...) { ++crashes; }
            }
        };

        std::vector<std::thread> threads;
        threads.reserve(THREADS);
        for (int t = 0; t < THREADS; ++t) threads.emplace_back(worker, t);
        for (auto& th : threads) th.join();

        ASSERT_EQ(crashes.load(), 0);
    });
}

// =============================================================================
// 30. Idempotency and double-operation invariants
// =============================================================================

static void test_idempotency() {
    run_suite("idempotency / double-op invariants", [] {
        ChronoCache c;

        // Double set — second wins
        c.set("idem", "first");
        c.set("idem", "second");
        ASSERT_EQ(*c.get("idem"), "second");

        // Double del — first succeeds, second fails
        c.set("dd", "v");
        ASSERT_TRUE(c.del("dd"));
        ASSERT_FALSE(c.del("dd"));

        // Double expire — second wins
        c.set("de", "v");
        c.expire("de", ms(5000));
        c.expire("de", ms(50));
        sleep_ms(100);
        ASSERT_FALSE(c.get("de").has_value());

        // Double persist — both succeed on a key with TTL; second is on no-TTL key
        c.set("dp", "v", ms(5000));
        ASSERT_TRUE(c.persist("dp"));
        ASSERT_TRUE(c.persist("dp")); // already no TTL — still returns true (key exists)

        // persist on non-existent
        ASSERT_FALSE(c.persist("ghost_idem"));

        // zadd same member twice with same score — both return false after first
        c.zadd("zi", 1.0, "a");
        ASSERT_FALSE(c.zadd("zi", 1.0, "a")); // no-op

        // zrem non-existent twice
        ASSERT_FALSE(c.zrem("zi", "ghost"));
        ASSERT_FALSE(c.zrem("zi", "ghost"));

        // zrem then re-add
        c.zadd("zi2", 5.0, "x");
        c.zrem("zi2", "x");
        ASSERT_TRUE(c.zadd("zi2", 5.0, "x")); // fresh insert → true
    });
}

// =============================================================================
// 31. Independent instances don't share state
// =============================================================================

static void test_independent_instances() {
    run_suite("independent instances", [] {
        ChronoCache a, b;

        a.set("key", "from_a");
        b.set("key", "from_b");

        ASSERT_EQ(*a.get("key"), "from_a");
        ASSERT_EQ(*b.get("key"), "from_b");

        a.del("key");
        ASSERT_FALSE(a.get("key").has_value());
        ASSERT_EQ(*b.get("key"), "from_b");

        a.zadd("z", 1.0, "m");
        ASSERT_FALSE(b.zscore("z", "m").has_value());
    });
}

// =============================================================================
// main
// =============================================================================

int main() {
    // Basic KV operations
    test_set_get_basic();
    test_set_get_ttl();
    test_del();
    test_expire();
    test_persist();
    test_pttl();

    // Sorted set operations
    test_sorted_set_basic();
    test_sorted_set_ordering();
    test_sorted_set_multiple_sets();
    test_sorted_set_edge_cases();
    test_sorted_set_single_element();
    test_sorted_set_score_update_rank();
    test_sorted_set_large();

    // TTL interaction edge cases
    test_ttl_interactions();

    // Scale / resize
    test_large_insert();

    // Concurrent correctness
    test_concurrent_disjoint_keys();
    test_concurrent_same_key();
    test_concurrent_set_get();
    test_concurrent_del_get();
    test_concurrent_expire_get();
    test_concurrent_sorted_set();
    test_concurrent_resize_stress();
    test_concurrent_mixed_ops();
    test_ttl_concurrent_expiry();
    test_concurrent_set_del();
    test_concurrent_expire_persist();

    // Miscellaneous
    test_pttl_monotone();
    test_idempotency();
    test_independent_instances();

    // Stress
    test_stress_random_kv();
    test_stress_random_sorted_set();

    print_results();
    return (g_failed == 0) ? 0 : 1;
}
