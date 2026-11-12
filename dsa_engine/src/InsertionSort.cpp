#include "Sorting.h"

#include <vector>

// This file contains one sorting algorithm: insertion sort.
namespace Sorting {

void insertionSortByQuantity(std::vector<Food>& data) {
    for (int i = 1; i < static_cast<int>(data.size()); ++i) {
        Food key = data[i];
        int j = i - 1;

        while (j >= 0 && (key.quantity < data[j].quantity
                || (key.quantity == data[j].quantity
                    && key.id < data[j].id))) {
            data[j + 1] = data[j];
            --j;
        }
        data[j + 1] = key;
    }
}

} // namespace Sorting
