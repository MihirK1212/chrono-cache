#ifndef AVL_TREE_H
#define AVL_TREE_H

#include <algorithm>
#include <iostream>
#include <string>
#include <utility>


struct AVLTreeNodeValue {
    private:

    std::string member;
    double score;

    public:

    AVLTreeNodeValue(std::string m, double s) : member(std::move(m)), score(s) {}

    const std::string& get_member() const {
        return member;
    }

    double get_score() const {
        return score;
    }

    bool operator<(const AVLTreeNodeValue& other) const {
        if(score == other.score) {
            return member < other.member;
        }
        return score < other.score;
    }

    bool operator>(const AVLTreeNodeValue& other) const {
        return other < *this;
    }

    bool operator==(const AVLTreeNodeValue& other) const {
        return !(*this < other) && !(other < *this);
    }
};

struct AVLTreeNode {
    AVLTreeNodeValue value;

    AVLTreeNode* left;
    AVLTreeNode* right;

    int height;
    int subtree_size; // the number of nodes in the subtree rooted at this node

    AVLTreeNode(const AVLTreeNodeValue& value)
        : value(value), left(nullptr), right(nullptr),
          height(0), subtree_size(1) {}

    int get_balance() const {
        int l_height = (left  == nullptr) ? -1 : left->height;
        int r_height = (right == nullptr) ? -1 : right->height;
        return l_height - r_height;
    }

    void recompute_height() {
        int l_h = (left  == nullptr) ? -1 : left->height;
        int r_h = (right == nullptr) ? -1 : right->height;
        height = 1 + std::max(l_h, r_h);
    }

    void recompute_subtree_size() {
        int l_s = (left  == nullptr) ? 0 : left->subtree_size;
        int r_s = (right == nullptr) ? 0 : right->subtree_size;
        subtree_size = 1 + l_s + r_s;
    }
};

class AVLTree {
    private:

    AVLTreeNode* root;

    static AVLTreeNode* _insert(AVLTreeNode* root, const AVLTreeNodeValue& value);
    static AVLTreeNode* _erase(AVLTreeNode* root, const AVLTreeNodeValue& value);

    static AVLTreeNode* _rebalance(AVLTreeNode* node);
    static AVLTreeNode* _right_rotate(AVLTreeNode* node);
    static AVLTreeNode* _left_rotate(AVLTreeNode* node);

    static int _rank(AVLTreeNode* node, const AVLTreeNodeValue& value);
    static void _destroy(AVLTreeNode* node);

    public:

    AVLTree();
    ~AVLTree();
    AVLTree(const AVLTree&) = delete;
    AVLTree& operator=(const AVLTree&) = delete;

    bool insert(const AVLTreeNodeValue& value);
    bool erase(const AVLTreeNodeValue& value);

    int rank(const AVLTreeNodeValue& value) const;
};

#endif
