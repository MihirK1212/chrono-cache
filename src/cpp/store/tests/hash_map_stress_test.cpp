#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "../../store/hash_map.h"

// ── Test harness ─────────────────────────────────────────────────────────────

static int g_passed = 0;
static int g_failed = 0;
static std::string g_suite;

void begin_suite(const std::string& name) {
    g_suite = name;
    std::cout << "\n=== " << name << " ===" << std::endl;
}

void check(bool condition, const std::string& label) {
    if (condition) {
        std::cout << "  [PASS] " << label << "\n";
        ++g_passed;
    } else {
        std::cerr << "  [FAIL] " << label << "  (suite: " << g_suite << ")\n";
        ++g_failed;
    }
}

template<typename T>
void check_eq(const T& actual, const T& expected, const std::string& label) {
    if (actual == expected) {
        std::cout << "  [PASS] " << label << "\n";
        ++g_passed;
    } else {
        std::ostringstream msg;
        msg << label << "  expected='" << expected << "'  got='" << actual << "'";
        std::cerr << "  [FAIL] " << msg.str() << "  (suite: " << g_suite << ")\n";
        ++g_failed;
    }
}

// Returns the value if the key exists, otherwise a sentinel.
template<typename K, typename V>
V safe_get(ChronCacheHashMap<K, V>& m, const K& key, const V& sentinel) {
    try { return m.get(key); }
    catch (const std::runtime_error&) { return sentinel; }
}

// ── Unit tests ───────────────────────────────────────────────────────────────

void test_basic_set_get() {
    begin_suite("Basic set / get");

    ChronCacheHashMap<std::string, std::string> m(16);

    m.set("alpha", "AAA");
    m.set("beta",  "BBB");
    m.set("gamma", "CCC");

    check_eq(m.get("alpha"), std::string("AAA"), "get alpha");
    check_eq(m.get("beta"),  std::string("BBB"), "get beta");
    check_eq(m.get("gamma"), std::string("CCC"), "get gamma");
}

void test_get_missing_key_throws() {
    begin_suite("get missing key throws");

    ChronCacheHashMap<std::string, std::string> m(16);
    m.set("exists", "yes");

    bool threw = false;
    try { m.get("missing"); }
    catch (const std::runtime_error&) { threw = true; }

    check(threw, "get('missing') throws runtime_error");
}

void test_overwrite_existing_key() {
    begin_suite("Overwrite existing key");

    ChronCacheHashMap<std::string, std::string> m(16);
    m.set("key", "v1");
    check_eq(m.get("key"), std::string("v1"), "initial value is v1");

    m.set("key", "v2");
    check_eq(m.get("key"), std::string("v2"), "after overwrite value is v2");

    m.set("key", "v3");
    check_eq(m.get("key"), std::string("v3"), "after second overwrite value is v3");
}

void test_remove() {
    begin_suite("Remove");

    ChronCacheHashMap<std::string, std::string> m(16);
    m.set("a", "1");
    m.set("b", "2");
    m.set("c", "3");

    check(m.remove("b"),  "remove existing key returns true");
    check(!m.remove("b"), "remove same key again returns false");
    check(!m.remove("z"), "remove never-inserted key returns false");

    bool threw = false;
    try { m.get("b"); }
    catch (const std::runtime_error&) { threw = true; }
    check(threw, "get removed key throws");

    check_eq(m.get("a"), std::string("1"), "sibling 'a' still accessible");
    check_eq(m.get("c"), std::string("3"), "sibling 'c' still accessible");
}

void test_remove_all_then_reinsert() {
    begin_suite("Remove all entries then re-insert");

    ChronCacheHashMap<std::string, int> m(8);
    const int N = 20;

    for (int i = 0; i < N; ++i)
        m.set("k" + std::to_string(i), i);

    for (int i = 0; i < N; ++i)
        check(m.remove("k" + std::to_string(i)), "remove k" + std::to_string(i));

    bool all_gone = true;
    for (int i = 0; i < N; ++i) {
        if (safe_get(m, std::string("k" + std::to_string(i)), -1) != -1)
            all_gone = false;
    }
    check(all_gone, "all entries gone after full removal");

    for (int i = 0; i < N; ++i)
        m.set("k" + std::to_string(i), i * 10);

    bool all_correct = true;
    for (int i = 0; i < N; ++i) {
        if (m.get("k" + std::to_string(i)) != i * 10) { all_correct = false; }
    }
    check(all_correct, "re-inserted entries all have new values");
}

void test_manual_resize() {
    begin_suite("Manual resize");

    ChronCacheHashMap<std::string, int> m(4);
    m.set("x", 10);
    m.set("y", 20);
    m.set("z", 30);

    m.resize(32);

    check_eq(m.get("x"), 10, "x survives manual resize");
    check_eq(m.get("y"), 20, "y survives manual resize");
    check_eq(m.get("z"), 30, "z survives manual resize");
}

void test_resize_error_cases() {
    begin_suite("resize error cases");

    ChronCacheHashMap<std::string, int> m(8);
    m.set("a", 1);

    {
        bool threw = false;
        try { m.resize(4); }
        catch (const std::runtime_error&) { threw = true; }
        check(threw, "resize to smaller capacity throws");
    }
    {
        bool threw = false;
        try { m.resize(8); }
        catch (const std::runtime_error&) { threw = true; }
        check(threw, "resize to equal capacity throws");
    }
    {
        bool threw = false;
        try { m.resize(12); }
        catch (const std::runtime_error&) { threw = true; }
        check(threw, "resize to non-power-of-2 throws");
    }
}

void test_integer_key_map() {
    begin_suite("Integer key map");

    ChronCacheHashMap<int, int> m(8);
    const int N = 50;

    for (int i = 0; i < N; ++i) m.set(i, i * i);

    bool all_ok = true;
    for (int i = 0; i < N; ++i) {
        if (m.get(i) != i * i) { all_ok = false; }
    }
    check(all_ok, "all int->int entries correct after potential auto-resize");

    for (int i = 0; i < N; i += 2) m.remove(i);

    bool evens_gone = true, odds_ok = true;
    for (int i = 0; i < N; ++i) {
        if (i % 2 == 0) {
            if (safe_get(m, i, -1) != -1) evens_gone = false;
        } else {
            if (m.get(i) != i * i) odds_ok = false;
        }
    }
    check(evens_gone, "even keys removed");
    check(odds_ok,    "odd keys still correct");
}

// ── Stress tests ─────────────────────────────────────────────────────────────
// Each stress test uses std::unordered_map as a ground-truth oracle.

void stress_test_large_volume() {
    begin_suite("Large-volume stress (N=2000, initial capacity=4)");

    // Start tiny so multiple auto-resizes are forced.
    ChronCacheHashMap<std::string, std::string> m(4);
    std::unordered_map<std::string, std::string> ref;

    const int N = 2000;

    // Phase 1: insert N entries
    for (int i = 0; i < N; ++i) {
        std::string k = "key_" + std::to_string(i);
        std::string v = "val_" + std::to_string(i);
        m.set(k, v);
        ref[k] = v;
    }

    bool all_inserted_ok = true;
    for (auto& [k, v] : ref) {
        if (m.get(k) != v) { all_inserted_ok = false; break; }
    }
    check(all_inserted_ok, "Phase 1: all " + std::to_string(N) + " inserts correct after auto-resizes");

    // Phase 2: overwrite every other entry
    for (int i = 0; i < N; i += 2) {
        std::string k = "key_" + std::to_string(i);
        std::string v = "updated_" + std::to_string(i);
        m.set(k, v);
        ref[k] = v;
    }

    bool all_updates_ok = true;
    for (auto& [k, v] : ref) {
        if (m.get(k) != v) { all_updates_ok = false; break; }
    }
    check(all_updates_ok, "Phase 2: all updates correct");

    // Phase 3: remove the first third
    int removed = 0;
    for (int i = 0; i < N / 3; ++i) {
        std::string k = "key_" + std::to_string(i);
        if (m.remove(k)) {
            ref.erase(k);
            ++removed;
        }
    }

    bool removed_gone = true;
    for (int i = 0; i < N / 3; ++i) {
        std::string k = "key_" + std::to_string(i);
        if (ref.count(k) == 0 && safe_get(m, k, std::string("")) != "") {
            removed_gone = false; break;
        }
    }
    check(removed_gone, "Phase 3: removed keys are gone (" + std::to_string(removed) + " removed)");

    bool survivors_ok = true;
    for (auto& [k, v] : ref) {
        if (m.get(k) != v) { survivors_ok = false; break; }
    }
    check(survivors_ok, "Phase 3: surviving keys still correct (" + std::to_string(ref.size()) + " entries)");

    // Phase 4: manual resize on live data.
    // After 2000 inserts with threshold=0.75, auto-resize lands at cap=4096,
    // so the manual target must be strictly larger.
    m.resize(8192);
    bool post_resize_ok = true;
    for (auto& [k, v] : ref) {
        if (m.get(k) != v) { post_resize_ok = false; break; }
    }
    check(post_resize_ok, "Phase 4: all entries correct after manual resize to 8192");

    // Phase 5: insert fresh entries into the resized map
    for (int i = N; i < N + 500; ++i) {
        std::string k = "key_" + std::to_string(i);
        std::string v = "fresh_" + std::to_string(i);
        m.set(k, v);
        ref[k] = v;
    }

    bool fresh_ok = true;
    for (int i = N; i < N + 500; ++i) {
        std::string k = "key_" + std::to_string(i);
        if (m.get(k) != ref[k]) { fresh_ok = false; break; }
    }
    check(fresh_ok, "Phase 5: 500 fresh entries correct after resize");
}

void stress_test_collision_heavy() {
    begin_suite("Collision-heavy: many keys into tiny initial capacity");

    ChronCacheHashMap<int, int> m(4);
    std::unordered_map<int, int> ref;

    for (int i = 1; i <= 200; ++i) {
        m.set(i, i * 3);
        ref[i] = i * 3;
    }

    bool ok = true;
    for (auto& [k, v] : ref) {
        if (m.get(k) != v) { ok = false; break; }
    }
    check(ok, "200 int entries correct after repeated auto-resize from cap=4");

    // Remove in reverse order, spot-checking neighbours at each step
    for (int i = 200; i >= 1; --i) {
        m.remove(i);
        ref.erase(i);
        if (i > 1 && ref.count(i - 1)) {
            if (m.get(i - 1) != ref[i - 1]) { ok = false; break; }
        }
    }
    check(ok, "reverse-order removal keeps neighbours intact throughout");
}

void stress_test_alternating_insert_delete() {
    begin_suite("Alternating insert/delete to exercise chain relinking");

    ChronCacheHashMap<std::string, int> m(8);
    std::unordered_map<std::string, int> ref;

    const int ROUNDS = 300;
    for (int r = 0; r < ROUNDS; ++r) {
        for (int j = 0; j < 2; ++j) {
            std::string k = "r" + std::to_string(r) + "_" + std::to_string(j);
            m.set(k, r * 100 + j);
            ref[k] = r * 100 + j;
        }
        if (r > 0) {
            std::string old = "r" + std::to_string(r - 1) + "_0";
            if (ref.count(old)) {
                m.remove(old);
                ref.erase(old);
            }
        }
    }

    bool ok = true;
    for (auto& [k, v] : ref) {
        if (m.get(k) != v) { ok = false; break; }
    }
    check(ok, "all surviving entries correct after " + std::to_string(ROUNDS) + " alternating rounds");

    std::vector<std::string> keys;
    for (auto& [k, _] : ref) keys.push_back(k);
    for (auto& k : keys) m.remove(k);

    bool all_gone = true;
    for (auto& k : keys) {
        if (safe_get(m, k, -1) != -1) { all_gone = false; break; }
    }
    check(all_gone, "map empty after removing all remaining entries");
}

// ── Entry point ───────────────────────────────────────────────────────────────

int main() {
    try {
        test_basic_set_get();
        test_get_missing_key_throws();
        test_overwrite_existing_key();
        test_remove();
        test_remove_all_then_reinsert();
        test_manual_resize();
        test_resize_error_cases();
        test_integer_key_map();

        stress_test_large_volume();
        stress_test_collision_heavy();
        stress_test_alternating_insert_delete();
    } catch (const std::exception& e) {
        std::cerr << "\n[CRASH] Uncaught exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "\n[CRASH] Unknown uncaught exception" << std::endl;
        return 1;
    }

    std::cout << "\n";
    std::cout << "==============================" << std::endl;
    std::cout << "  Results: " << g_passed << " passed, " << g_failed << " failed" << std::endl;
    std::cout << "==============================" << std::endl;

    return g_failed == 0 ? 0 : 1;
}
