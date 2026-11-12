#include "Searching.h"

#include <vector>

// This file contains one searching algorithm: binary search.
namespace Searching {

int binarySearchById(const std::vector<Food>& sortedFoods, int id) {
    int low = 0;
    int high = static_cast<int>(sortedFoods.size()) - 1;

    while (low <= high) {
        int middle = low + (high - low) / 2;

        if (sortedFoods[middle].id == id) {
            return middle;
        }
        if (sortedFoods[middle].id < id) {
            low = middle + 1;
        } else {
            high = middle - 1;
        }
    }
    return -1;
}

} // namespace Searching
