#include "tree.h"

AVLTree::AVLTree() : root(nullptr) {}

AVLTree::~AVLTree() {
    _destroy(root);
}

bool AVLTree::insert(const AVLTreeNodeValue& value) {
    int size_before = (root == nullptr) ? 0 : root->subtree_size;
    root = _insert(root, value);
    int size_after = (root == nullptr) ? 0 : root->subtree_size;
    return size_after > size_before;
}

bool AVLTree::erase(const AVLTreeNodeValue& value) {
    int size_before = (root == nullptr) ? 0 : root->subtree_size;
    root = _erase(root, value);
    int size_after = (root == nullptr) ? 0 : root->subtree_size;
    return size_after < size_before;
}

AVLTreeNode* AVLTree::_insert(AVLTreeNode* root, const AVLTreeNodeValue& value) {
    if(root == nullptr) {
        return new AVLTreeNode(value);
    }

    if(value < root->value) {
        root->left = _insert(root->left, value);
    } else if(value > root->value) {
        root->right = _insert(root->right, value);
    } else {
        return root; // exact duplicate — no-op
    }

    root->recompute_height();
    root->recompute_subtree_size();
    return _rebalance(root);
}

AVLTreeNode* AVLTree::_erase(AVLTreeNode* root, const AVLTreeNodeValue& value) {
    if(root == nullptr) {
        return root;
    }

    AVLTreeNode* result = root;

    if(value < root->value) {
        root->left = _erase(root->left, value);
    }
    else if(value > root->value) {
        root->right = _erase(root->right, value);
    }
    else {
        if(root->left == nullptr && root->right == nullptr) {
            delete root;
            return nullptr;
        }
        else if(root->left == nullptr) {
            result = root->right;
            delete root;
        }
        else if(root->right == nullptr) {
            result = root->left;
            delete root;
        }
        else {
            // Find in-order successor: leftmost node in right subtree
            AVLTreeNode* successor = root->right;
            while(successor->left != nullptr) {
                successor = successor->left;
            }
            root->value = successor->value;
            root->right = _erase(root->right, successor->value);
        }
    }

    if(result == nullptr) {
        return result;
    }

    result->recompute_height();
    result->recompute_subtree_size();
    return _rebalance(result);
}

AVLTreeNode* AVLTree::_rebalance(AVLTreeNode* root) {
    if(root == nullptr) {
        return root;
    }

    int balance = root->get_balance();

    if(balance > 1 && root->left != nullptr && root->left->get_balance() >= 0)          // Left-Left
        return _right_rotate(root);

    if(balance < -1 && root->right != nullptr && root->right->get_balance() <= 0)        // Right-Right
        return _left_rotate(root);

    if(balance > 1 && root->left != nullptr && root->left->get_balance() < 0) {         // Left-Right
        root->left = _left_rotate(root->left);
        return _right_rotate(root);
    }

    if(balance < -1 && root->right != nullptr && root->right->get_balance() > 0) {       // Right-Left
        root->right = _right_rotate(root->right);
        return _left_rotate(root);
    }

    return root;
}

AVLTreeNode* AVLTree::_right_rotate(AVLTreeNode* n1) {
    if(n1 == nullptr || n1->left == nullptr) {
        return n1;
    }

    AVLTreeNode* n2 = n1->left;
    AVLTreeNode* T = n2->right;

    n2->right = n1;
    n1->left = T;

    n1->recompute_height();
    n2->recompute_height();

    n1->recompute_subtree_size();
    n2->recompute_subtree_size();

    return n2;
}

AVLTreeNode* AVLTree::_left_rotate(AVLTreeNode* n1) {
    if(n1 == nullptr || n1->right == nullptr) {
        return n1;
    }

    AVLTreeNode* n2 = n1->right;
    AVLTreeNode* T = n2->left;

    n2->left = n1;
    n1->right = T;

    n1->recompute_height();
    n2->recompute_height();

    n1->recompute_subtree_size();
    n2->recompute_subtree_size();
    
    return n2;
}

int AVLTree::rank(const AVLTreeNodeValue& value) const {
    return _rank(root, value);
}

int AVLTree::_rank(AVLTreeNode* node, const AVLTreeNodeValue& value) {
    if(node == nullptr) {
        return 0;
    }

    if(value < node->value) {
        return _rank(node->left, value);
    }
    else if(value > node->value) {
        return (node->left == nullptr ? 0 : node->left->subtree_size) + 1 + _rank(node->right, value);
    }
    else {
        return node->left == nullptr ? 0 : node->left->subtree_size;
    }
}   

void AVLTree::_destroy(AVLTreeNode* node) {
    if(node == nullptr) {
        return;
    }

    _destroy(node->left);
    _destroy(node->right);
    delete node;
}