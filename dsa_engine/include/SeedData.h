#pragma once

#include <unordered_map>

#include "BST.h"
#include "Food.h"
#include "SinglyLinkedList.h"

namespace SeedData {

// Load the sample listings. Dates are relative to today so the demo never
// starts with every item already expired.
void load(
    FoodList& foods,
    std::unordered_map<int, Food>& index,
    FoodBST& tree
);

} // namespace SeedData
