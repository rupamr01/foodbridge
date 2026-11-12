#pragma once

#include <unordered_map>

#include "BST.h"
#include "Food.h"
#include "SinglyLinkedList.h"

// Keeps the three food containers in sync:
//   - linked list for traversal
//   - hash map for O(1) id lookup
//   - BST for ordered id lookup
void addFoodToStore(
    FoodList& foods,
    std::unordered_map<int, Food>& index,
    FoodBST& tree,
    const Food& food
);

bool removeFoodFromStore(
    FoodList& foods,
    std::unordered_map<int, Food>& index,
    FoodBST& tree,
    int id
);
