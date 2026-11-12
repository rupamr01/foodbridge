#pragma once

#include <string>
#include <vector>

#include "Food.h"

// Each searching algorithm lives in its own .cpp file:
//   LinearSearch.cpp  - linear search
//   BinarySearch.cpp  - binary search
namespace Searching {

// Lower-case a string. Used for case-insensitive name/category matching.
std::string toLower(std::string text);

// Linear search: checks every listing and collects all name matches.
std::vector<Food> linearSearchByName(
    const std::vector<Food>& foods,
    const std::string& query
);

// Binary search on a vector already sorted by id.
// Returns the index, or -1 when the id is not present.
int binarySearchById(const std::vector<Food>& sortedFoods, int id);

} // namespace Searching
