#pragma once

#include <cstddef>
#include <vector>

#include "Food.h"

// LIFO stack used for "undo last added listing".
class FoodStack {
private:
    std::vector<Food> items;

public:
    void push(const Food& value);
    bool pop(Food& out);
    bool empty() const;
    std::size_t size() const;
    const Food& top() const;
};
