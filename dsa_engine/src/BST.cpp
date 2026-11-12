#include "BST.h"

#include <iostream>

FoodBST::FoodBST() : root(nullptr) {}

FoodBST::~FoodBST() {
    clear();
}

FoodBST::Node* FoodBST::insertNode(Node* node, const Food& food) {
    if (node == nullptr) {
        return new Node(food);
    }
    if (food.id < node->data.id) {
        node->left = insertNode(node->left, food);
    } else if (food.id > node->data.id) {
        node->right = insertNode(node->right, food);
    }
    return node;
}

const Food* FoodBST::findNode(Node* node, int id) const {
    if (node == nullptr) {
        return nullptr;
    }
    if (id == node->data.id) {
        return &node->data;
    }
    if (id < node->data.id) {
        return findNode(node->left, id);
    }
    return findNode(node->right, id);
}

void FoodBST::inorderNode(Node* node) const {
    if (node == nullptr) {
        return;
    }
    inorderNode(node->left);
    std::cout << "  [" << node->data.id << "] "
              << node->data.name
              << "  (best before " << node->data.bestBefore << ")\n";
    inorderNode(node->right);
}

void FoodBST::clearNode(Node* node) {
    if (node == nullptr) {
        return;
    }
    clearNode(node->left);
    clearNode(node->right);
    delete node;
}

void FoodBST::insert(const Food& food) {
    root = insertNode(root, food);
}

const Food* FoodBST::find(int id) const {
    return findNode(root, id);
}

void FoodBST::inorder() const {
    inorderNode(root);
}

bool FoodBST::empty() const {
    return root == nullptr;
}

void FoodBST::clear() {
    clearNode(root);
    root = nullptr;
}

void FoodBST::rebuildFrom(const std::vector<Food>& foods) {
    clear();
    for (const Food& food : foods) {
        insert(food);
    }
}
