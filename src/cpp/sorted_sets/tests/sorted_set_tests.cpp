#include <algorithm>
#include <iostream>
#include <limits>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "../sorted_set.h"

namespace {

using ReferenceStore = std::unordered_map<std::string, std::unordered_map<std::string, double>>;

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void expect_optional_double(
    const std::optional<double>& actual,
    double expected,
    const std::string& context
) {
    require(actual.has_value(), context + ": expected a score");
    require(actual.value() == expected, context + ": unexpected score");
}

void expect_optional_int(
    const std::optional<int>& actual,
    int expected,
    const std::string& context
) {
    require(actual.has_value(), context + ": expected a rank");
    require(actual.value() == expected, context + ": unexpected rank");
}

void expect_empty_score(const std::optional<double>& actual, const std::string& context) {
    require(!actual.has_value(), context + ": expected missing score");
}

void expect_empty_rank(const std::optional<int>& actual, const std::string& context) {
    require(!actual.has_value(), context + ": expected missing rank");
}

int reference_rank(const ReferenceStore& reference, const std::string& key, const std::string& member) {
    const auto key_iter = reference.find(key);
    require(key_iter != reference.end(), "reference_rank called for missing key");

    const auto member_iter = key_iter->second.find(member);
    require(member_iter != key_iter->second.end(), "reference_rank called for missing member");

    std::vector<std::pair<double, std::string>> ordered;
    ordered.reserve(key_iter->second.size());
    for (const auto& entry : key_iter->second) {
        ordered.emplace_back(entry.second, entry.first);
    }

    std::sort(ordered.begin(), ordered.end(), [](const auto& lhs, const auto& rhs) {
        if (lhs.first == rhs.first) {
            return lhs.second < rhs.second;
        }
        return lhs.first < rhs.first;
    });

    for (size_t i = 0; i < ordered.size(); i++) {
        if (ordered[i].second == member) {
            return static_cast<int>(i);
        }
    }

    throw std::runtime_error("reference member disappeared while ranking");
}

void verify_member(
    const SortedSetsAPI& api,
    const ReferenceStore& reference,
    const std::string& key,
    const std::string& member
) {
    const auto key_iter = reference.find(key);
    if (key_iter == reference.end()) {
        expect_empty_score(api.zscore(key, member), "missing key score for " + key + "/" + member);
        expect_empty_rank(api.zrank(key, member), "missing key rank for " + key + "/" + member);
        return;
    }

    const auto member_iter = key_iter->second.find(member);
    if (member_iter == key_iter->second.end()) {
        expect_empty_score(api.zscore(key, member), "missing member score for " + key + "/" + member);
        expect_empty_rank(api.zrank(key, member), "missing member rank for " + key + "/" + member);
        return;
    }

    expect_optional_double(api.zscore(key, member), member_iter->second, "score for " + key + "/" + member);
    expect_optional_int(api.zrank(key, member), reference_rank(reference, key, member), "rank for " + key + "/" + member);
}

void verify_key(
    const SortedSetsAPI& api,
    const ReferenceStore& reference,
    const std::string& key,
    const std::vector<std::string>& members
) {
    for (const std::string& member : members) {
        verify_member(api, reference, key, member);
    }
}

void test_missing_keys_and_empty_strings() {
    SortedSetsAPI api;

    expect_empty_score(api.zscore("missing", "member"), "zscore on missing key");
    expect_empty_rank(api.zrank("missing", "member"), "zrank on missing key");
    require(!api.zrem("missing", "member"), "zrem should return false for missing key");

    require(api.zadd("", 0.0, ""), "zadd should insert empty key/member");
    expect_optional_double(api.zscore("", ""), 0.0, "empty key/member score");
    expect_optional_int(api.zrank("", ""), 0, "empty key/member rank");

    require(api.zrem("", ""), "zrem should remove empty key/member");
    expect_empty_score(api.zscore("", ""), "removed empty key/member score");
    expect_empty_rank(api.zrank("", ""), "removed empty key/member rank");
    require(!api.zrem("", ""), "zrem should return false for already removed member");
}

void test_zadd_return_values_and_updates() {
    SortedSetsAPI api;

    require(api.zadd("scores", 10.0, "alice"), "first zadd should report insertion");
    require(!api.zadd("scores", 10.0, "alice"), "same score zadd should report no insertion");
    expect_optional_double(api.zscore("scores", "alice"), 10.0, "alice score after duplicate zadd");
    expect_optional_int(api.zrank("scores", "alice"), 0, "alice rank after duplicate zadd");

    require(!api.zadd("scores", 5.0, "alice"), "score update should not report insertion");
    expect_optional_double(api.zscore("scores", "alice"), 5.0, "alice score after update");
    expect_optional_int(api.zrank("scores", "alice"), 0, "alice rank after update");

    require(api.zadd("scores", 5.0, "bob"), "new member with tied score should report insertion");
    expect_optional_int(api.zrank("scores", "alice"), 0, "alice rank with tied bob");
    expect_optional_int(api.zrank("scores", "bob"), 1, "bob rank with tied alice");
}

void test_score_ordering_ties_and_extreme_values() {
    SortedSetsAPI api;

    require(api.zadd("mixed", 2.0, "beta"), "insert beta");
    require(api.zadd("mixed", 1.0, "alpha"), "insert alpha");
    require(api.zadd("mixed", 1.0, "delta"), "insert delta");
    require(api.zadd("mixed", -5.0, "gamma"), "insert gamma");
    require(api.zadd("mixed", std::numeric_limits<double>::infinity(), "epsilon"), "insert epsilon");
    require(api.zadd("mixed", -std::numeric_limits<double>::infinity(), "zeta"), "insert zeta");
    require(api.zadd("mixed", 1.0, ""), "insert empty member");
    require(api.zadd("mixed", -0.0, "zero"), "insert negative zero");

    expect_optional_int(api.zrank("mixed", "zeta"), 0, "negative infinity ranks first");
    expect_optional_int(api.zrank("mixed", "gamma"), 1, "negative score rank");
    expect_optional_int(api.zrank("mixed", "zero"), 2, "negative zero rank");
    expect_optional_int(api.zrank("mixed", ""), 3, "empty member tie rank");
    expect_optional_int(api.zrank("mixed", "alpha"), 4, "alpha tie rank");
    expect_optional_int(api.zrank("mixed", "delta"), 5, "delta tie rank");
    expect_optional_int(api.zrank("mixed", "beta"), 6, "beta rank");
    expect_optional_int(api.zrank("mixed", "epsilon"), 7, "positive infinity ranks last");
}

void test_key_isolation_and_removal_reorders_ranks() {
    SortedSetsAPI api;

    require(api.zadd("one", 1.0, "shared"), "insert shared in key one");
    require(api.zadd("two", 100.0, "shared"), "insert shared in key two");
    require(api.zadd("one", 0.0, "first"), "insert first in key one");
    require(api.zadd("one", 2.0, "last"), "insert last in key one");

    expect_optional_double(api.zscore("one", "shared"), 1.0, "key one shared score");
    expect_optional_double(api.zscore("two", "shared"), 100.0, "key two shared score");
    expect_optional_int(api.zrank("one", "shared"), 1, "key one shared rank");
    expect_optional_int(api.zrank("two", "shared"), 0, "key two shared rank");

    require(api.zrem("one", "first"), "remove first");
    expect_optional_int(api.zrank("one", "shared"), 0, "shared rank after first removal");
    expect_optional_int(api.zrank("one", "last"), 1, "last rank after first removal");
    expect_optional_int(api.zrank("two", "shared"), 0, "key two remains isolated after key one removal");

    require(!api.zrem("one", "absent"), "removing absent member should be false");
    require(api.zrem("one", "shared"), "remove shared from key one");
    expect_empty_score(api.zscore("one", "shared"), "removed key one shared score");
    expect_optional_double(api.zscore("two", "shared"), 100.0, "key two shared still present");
}

void test_large_sequential_insert_update_and_remove() {
    SortedSetsAPI api;
    ReferenceStore reference;
    std::vector<std::string> members;

    for (int i = 0; i < 300; i++) {
        std::ostringstream member;
        member << "member-" << i;
        members.push_back(member.str());
        const double score = static_cast<double>((i * 37) % 101) - 50.0;
        require(api.zadd("bulk", score, members.back()), "bulk insert should report insertion");
        reference["bulk"][members.back()] = score;
    }
    verify_key(api, reference, "bulk", members);

    for (int i = 0; i < 300; i += 4) {
        const double score = static_cast<double>(200 - i);
        require(!api.zadd("bulk", score, members[i]), "bulk update should not report insertion");
        reference["bulk"][members[i]] = score;
    }
    verify_key(api, reference, "bulk", members);

    for (int i = 1; i < 300; i += 3) {
        require(api.zrem("bulk", members[i]), "bulk remove should report success");
        reference["bulk"].erase(members[i]);
    }
    verify_key(api, reference, "bulk", members);

    for (int i = 1; i < 300; i += 3) {
        require(!api.zrem("bulk", members[i]), "bulk second remove should report false");
    }
}

void test_deterministic_randomized_stress_against_reference() {
    SortedSetsAPI api;
    ReferenceStore reference;

    std::vector<std::string> keys;
    for (int i = 0; i < 10; i++) {
        keys.push_back("key-" + std::to_string(i));
    }
    keys.push_back("");

    std::vector<std::string> members;
    for (int i = 0; i < 75; i++) {
        members.push_back("member-" + std::to_string(i));
    }
    members.push_back("");
    members.push_back("punctuation !@#$%^&*()");

    std::mt19937 rng(0xC0FFEE);
    std::uniform_int_distribution<int> operation_dist(0, 99);
    std::uniform_int_distribution<int> key_dist(0, static_cast<int>(keys.size() - 1));
    std::uniform_int_distribution<int> member_dist(0, static_cast<int>(members.size() - 1));
    std::uniform_int_distribution<int> score_dist(-200, 200);

    for (int step = 0; step < 5000; step++) {
        const std::string& key = keys[key_dist(rng)];
        const std::string& member = members[member_dist(rng)];
        const int operation = operation_dist(rng);

        if (operation < 50) {
            const double score = static_cast<double>(score_dist(rng)) / 4.0;
            const bool was_present = reference[key].find(member) != reference[key].end();
            const bool inserted = api.zadd(key, score, member);
            require(inserted == !was_present, "random zadd return value mismatch");
            reference[key][member] = score;
        } else if (operation < 75) {
            const auto key_iter = reference.find(key);
            const bool was_present = key_iter != reference.end() &&
                key_iter->second.find(member) != key_iter->second.end();
            const bool removed = api.zrem(key, member);
            require(removed == was_present, "random zrem return value mismatch");
            if (was_present) {
                key_iter->second.erase(member);
            }
        } else {
            verify_member(api, reference, key, member);
        }

        verify_member(api, reference, key, member);
        if (step % 25 == 0) {
            for (const std::string& verify_key_name : keys) {
                verify_key(api, reference, verify_key_name, members);
            }
        }
    }

    for (const std::string& key : keys) {
        verify_key(api, reference, key, members);
    }
}

void test_score_update_crosses_ranks() {
    SortedSetsAPI api;

    require(api.zadd("ranks", 10.0, "a"), "insert a");
    require(api.zadd("ranks", 20.0, "b"), "insert b");
    require(api.zadd("ranks", 30.0, "c"), "insert c");
    require(api.zadd("ranks", 40.0, "d"), "insert d");
    require(api.zadd("ranks", 50.0, "e"), "insert e");

    expect_optional_int(api.zrank("ranks", "a"), 0, "a initial rank");
    expect_optional_int(api.zrank("ranks", "e"), 4, "e initial rank");

    // Move lowest-ranked member to the top
    require(!api.zadd("ranks", 60.0, "a"), "update a to 60");
    expect_optional_int(api.zrank("ranks", "b"), 0, "b rank after a moved up");
    expect_optional_int(api.zrank("ranks", "c"), 1, "c rank after a moved up");
    expect_optional_int(api.zrank("ranks", "d"), 2, "d rank after a moved up");
    expect_optional_int(api.zrank("ranks", "e"), 3, "e rank after a moved up");
    expect_optional_int(api.zrank("ranks", "a"), 4, "a rank after move to top");

    // Move highest-ranked member to the bottom
    require(!api.zadd("ranks", 5.0, "e"), "update e to 5");
    expect_optional_int(api.zrank("ranks", "e"), 0, "e rank after move to bottom");
    expect_optional_int(api.zrank("ranks", "b"), 1, "b rank after e moved down");
    expect_optional_int(api.zrank("ranks", "c"), 2, "c rank after e moved down");
    expect_optional_int(api.zrank("ranks", "d"), 3, "d rank after e moved down");
    expect_optional_int(api.zrank("ranks", "a"), 4, "a rank after e moved down");
}

void test_monotonic_inserts_and_removals() {
    // Purely ascending inserts stress right-right AVL rotations;
    // purely descending inserts stress left-left AVL rotations.
    SortedSetsAPI api;
    const int N = 100;

    for (int i = 0; i < N; i++) {
        require(api.zadd("asc", static_cast<double>(i), "m" + std::to_string(i)), "asc insert");
    }
    for (int i = 0; i < N; i++) {
        expect_optional_int(api.zrank("asc", "m" + std::to_string(i)), i, "asc rank of m" + std::to_string(i));
    }

    for (int i = N - 1; i >= 0; i--) {
        require(api.zadd("desc", static_cast<double>(i), "m" + std::to_string(i)), "desc insert");
    }
    for (int i = 0; i < N; i++) {
        expect_optional_int(api.zrank("desc", "m" + std::to_string(i)), i, "desc rank of m" + std::to_string(i));
    }

    // Remove every even-indexed element and check the surviving odd elements re-rank correctly
    for (int i = 0; i < N; i += 2) {
        require(api.zrem("asc", "m" + std::to_string(i)), "asc remove even");
    }
    int expected_rank = 0;
    for (int i = 1; i < N; i += 2) {
        expect_optional_int(
            api.zrank("asc", "m" + std::to_string(i)),
            expected_rank++,
            "asc odd rank after removals for m" + std::to_string(i)
        );
    }
}

void test_empty_set_reuse() {
    SortedSetsAPI api;

    require(api.zadd("reuse", 1.0, "x"), "insert x");
    require(api.zadd("reuse", 2.0, "y"), "insert y");
    require(api.zadd("reuse", 3.0, "z"), "insert z");

    require(api.zrem("reuse", "x"), "remove x");
    require(api.zrem("reuse", "y"), "remove y");
    require(api.zrem("reuse", "z"), "remove z");

    // SortedSet object persists in the store but the tree is empty
    expect_empty_score(api.zscore("reuse", "x"), "x score after set emptied");
    expect_empty_rank(api.zrank("reuse", "x"), "x rank after set emptied");
    require(!api.zrem("reuse", "x"), "zrem on emptied key should be false");

    // Re-populate the same key with different members
    require(api.zadd("reuse", 10.0, "p"), "re-add p");
    require(api.zadd("reuse", 5.0, "q"), "re-add q");
    require(api.zadd("reuse", 15.0, "r"), "re-add r");

    expect_optional_double(api.zscore("reuse", "q"), 5.0,  "q score after reuse");
    expect_optional_double(api.zscore("reuse", "p"), 10.0, "p score after reuse");
    expect_optional_double(api.zscore("reuse", "r"), 15.0, "r score after reuse");

    expect_optional_int(api.zrank("reuse", "q"), 0, "q rank after reuse");
    expect_optional_int(api.zrank("reuse", "p"), 1, "p rank after reuse");
    expect_optional_int(api.zrank("reuse", "r"), 2, "r rank after reuse");

    expect_empty_score(api.zscore("reuse", "x"), "x absent after reuse");
    expect_empty_score(api.zscore("reuse", "y"), "y absent after reuse");
    expect_empty_score(api.zscore("reuse", "z"), "z absent after reuse");
}

void run_test(const std::string& name, void (*test)()) {
    test();
    std::cout << "[PASS] " << name << '\n';
}

} // namespace

int main() {
    try {
        run_test("missing keys and empty strings", test_missing_keys_and_empty_strings);
        run_test("zadd return values and updates", test_zadd_return_values_and_updates);
        run_test("score ordering, ties, and extreme values", test_score_ordering_ties_and_extreme_values);
        run_test("key isolation and removal rank updates", test_key_isolation_and_removal_reorders_ranks);
        run_test("large sequential insert/update/remove", test_large_sequential_insert_update_and_remove);
        run_test("score update crosses ranks", test_score_update_crosses_ranks);
        run_test("monotonic inserts and removals", test_monotonic_inserts_and_removals);
        run_test("empty set reuse", test_empty_set_reuse);
        run_test("deterministic randomized stress", test_deterministic_randomized_stress_against_reference);
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }

    std::cout << "All SortedSetsAPI tests passed.\n";
    return 0;
}
