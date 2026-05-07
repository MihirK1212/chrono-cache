#include "sorted_set.h"

SortedSet::SortedSet() : tree(), member_score(SORTED_SET_INITIAL_MAP_CAPACITY) {}

SortedSetsAPI::SortedSetsAPI() : sorted_set_store(SORTED_SET_INITIAL_MAP_CAPACITY) {}

SortedSet& SortedSetsAPI::get_or_create_set(const std::string& key) {
    return sorted_set_store.get_or_add(key);
}

const SortedSet* SortedSetsAPI::find_set(const std::string& key) const {
    return sorted_set_store.get_ptr(key);
}

bool SortedSetsAPI::zadd(const std::string& key, double score, const std::string& member) {
    SortedSet& zset = get_or_create_set(key);
    auto existing = zset.member_score.get(member);

    if (existing.has_value()) {
        if (existing.value() == score) {
            return false;
        }
        zset.tree.erase(AVLTreeNodeValue(member, existing.value()));
        zset.tree.insert(AVLTreeNodeValue(member, score));
        zset.member_score.set(member, score);
        return false; // updated key
    }

    zset.tree.insert(AVLTreeNodeValue(member, score));
    zset.member_score.set(member, score);
    return true;
}

std::optional<double> SortedSetsAPI::zscore(
    const std::string& key,
    const std::string& member
) const {
    const SortedSet* zset = find_set(key);
    if (zset == nullptr) {
        return std::nullopt;
    }

    return zset->member_score.get(member);
}

bool SortedSetsAPI::zrem(const std::string& key, const std::string& member) {
    SortedSet* zset = sorted_set_store.get_ptr(key);
    if (zset == nullptr) {
        return false;
    }

    auto result = zset->member_score.get(member);
    if (!result.has_value()) {
        return false;
    }

    zset->tree.erase(AVLTreeNodeValue(member, result.value()));
    zset->member_score.remove(member);
    return true;
}

std::optional<int> SortedSetsAPI::zrank(const std::string& key, const std::string& member) const {
    const SortedSet* zset = find_set(key);
    if (zset == nullptr) {
        return std::nullopt;
    }

    auto result = zset->member_score.get(member);
    if (!result.has_value()) {
        return std::nullopt;
    }
    return zset->tree.rank(AVLTreeNodeValue(member, result.value()));
}
