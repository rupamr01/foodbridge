#include "Sorting.h"

#include <cstddef>
#include <utility>
#include <vector>

// This file contains one sorting algorithm: merge sort.
// The same merge logic is reused for Food vectors and MatchResult vectors.
namespace {

template <typename T, typename Compare>
void mergeSortRange(std::vector<T>& data, int left, int right, Compare less) {
    if (left >= right) {
        return;
    }

    int middle = left + (right - left) / 2;
    mergeSortRange(data, left, middle, less);
    mergeSortRange(data, middle + 1, right, less);

    std::vector<T> merged;
    merged.reserve(static_cast<std::size_t>(right - left + 1));

    int i = left;
    int j = middle + 1;

    while (i <= middle && j <= right) {
        if (less(data[j], data[i])) {
            merged.push_back(data[j]);
            ++j;
        } else {
            merged.push_back(data[i]);
            ++i;
        }
    }

    while (i <= middle) {
        merged.push_back(data[i]);
        ++i;
    }
    while (j <= right) {
        merged.push_back(data[j]);
        ++j;
    }

    for (int k = 0; k < static_cast<int>(merged.size()); ++k) {
        data[left + k] = merged[k];
    }
}

template <typename T, typename Compare>
void mergeSort(std::vector<T>& data, Compare less) {
    if (data.size() < 2) {
        return;
    }
    mergeSortRange(data, 0, static_cast<int>(data.size()) - 1, less);
}

} // namespace

namespace Sorting {

void mergeSortByExpiry(std::vector<Food>& data) {
    mergeSort(data, [](const Food& a, const Food& b) {
        if (a.bestBefore != b.bestBefore) {
            return a.bestBefore < b.bestBefore;
        }
        return a.id < b.id;
    });
}

void mergeSortById(std::vector<Food>& data) {
    mergeSort(data, [](const Food& a, const Food& b) {
        return a.id < b.id;
    });
}

void mergeSortByMatchScore(std::vector<MatchResult>& data) {
    mergeSort(data, [](const MatchResult& a, const MatchResult& b) {
        if (a.totalScore != b.totalScore) {
            return a.totalScore > b.totalScore;
        }
        if (a.daysLeft != b.daysLeft) {
            return a.daysLeft < b.daysLeft;
        }
        if (a.distance != b.distance) {
            return a.distance < b.distance;
        }
        return a.food.id < b.food.id;
    });
}

} // namespace Sorting
