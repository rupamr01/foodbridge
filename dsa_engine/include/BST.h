#pragma once

#include <vector>

#include "Food.h"

// Binary search tree keyed by food id.
// In-order traversal prints food ids in sorted order.
class FoodBST {
private:
    struct Node {
        Food data;
        Node* left;
        Node* right;
        explicit Node(const Food& food) : data(food), left(nullptr), right(nullptr) {}
    };

    Node* root;

    Node* insertNode(Node* node, const Food& food);
    const Food* findNode(Node* node, int id) const;
    void inorderNode(Node* node) const;
    void clearNode(Node* node);

public:
    FoodBST();
    ~FoodBST();

    FoodBST(const FoodBST&) = delete;
    FoodBST& operator=(const FoodBST&) = delete;

    void insert(const Food& food);
    const Food* find(int id) const;
    void inorder() const;
    bool empty() const;
    void clear();

    // Removing from a plain BST is more complex, so after a delete we simply
    // rebuild the tree from the current linked list.
    void rebuildFrom(const std::vector<Food>& foods);
};
