#pragma once

#include <cstddef>
#include <vector>

#include "Food.h"

// Comparator for the expiry min-heap: earliest date has the highest priority.
struct FoodExpiryLess {
    bool operator()(const Food& a, const Food& b) const;
};

// Binary min-heap of food listings.
class FoodMinHeap {
private:
    std::vector<Food> heap;
    FoodExpiryLess compare;

    void siftUp(int index);
    void siftDown(int index);

public:
    void push(const Food& value);
    bool pop(Food& out);

    bool empty() const;
    std::size_t size() const;
    const Food& top() const;
};
