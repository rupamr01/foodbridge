#pragma once

#include <vector>

#include "Food.h"

// Circular array queue used for NGO food requests (first in, first out).
class RequestQueue {
private:
    std::vector<FoodRequest> data;
    int capacity;
    int frontIndex;
    int count;

public:
    explicit RequestQueue(int capacityValue = 100);

    bool enqueue(const FoodRequest& value);
    bool dequeue(FoodRequest& out);

    bool empty() const;
    int size() const;
    int maxSize() const;
    const FoodRequest& front() const;
    std::vector<FoodRequest> toVector() const;
};
