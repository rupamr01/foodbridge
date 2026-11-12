#include "Stack.h"

void FoodStack::push(const Food& value) {
    items.push_back(value);
}

bool FoodStack::pop(Food& out) {
    if (items.empty()) {
        return false;
    }
    out = items.back();
    items.pop_back();
    return true;
}

bool FoodStack::empty() const {
    return items.empty();
}

std::size_t FoodStack::size() const {
    return items.size();
}

const Food& FoodStack::top() const {
    return items.back();
}
