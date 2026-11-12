#pragma once

#include <vector>

#include "Food.h"

// Singly linked list of food listings.
// Used to show insert, search, delete, and traversal without STL containers.
class FoodList {
private:
    struct Node {
        Food data;
        Node* next;
        explicit Node(const Food& food) : data(food), next(nullptr) {}
    };

    Node* head;
    int count;

public:
    FoodList();
    ~FoodList();

    FoodList(const FoodList&) = delete;
    FoodList& operator=(const FoodList&) = delete;

    void add(const Food& food);
    bool removeById(int id);
    const Food* findById(int id) const;
    std::vector<Food> toVector() const;

    int size() const;
    bool empty() const;
    void display() const;
    void clear();
};
