#include "Sorting.h"

#include <utility>
#include <vector>

// This file contains one sorting algorithm: quick sort.
namespace {

void quickSortRange(std::vector<Food>& data, int low, int high) {
    if (low >= high) {
        return;
    }

    Food pivot = data[high];
    int smaller = low;

    for (int current = low; current < high; ++current) {
        if (data[current].name < pivot.name
                || (data[current].name == pivot.name
                    && data[current].id < pivot.id)) {
            std::swap(data[smaller], data[current]);
            ++smaller;
        }
    }

    std::swap(data[smaller], data[high]);

    quickSortRange(data, low, smaller - 1);
    quickSortRange(data, smaller + 1, high);
}

} // namespace

namespace Sorting {

void quickSortByName(std::vector<Food>& data) {
    if (data.size() < 2) {
        return;
    }
    quickSortRange(data, 0, static_cast<int>(data.size()) - 1);
}

} // namespace Sorting
