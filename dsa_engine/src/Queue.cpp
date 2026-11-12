#include "Queue.h"

RequestQueue::RequestQueue(int capacityValue)
    : data(capacityValue > 0 ? capacityValue : 1),
      capacity(capacityValue > 0 ? capacityValue : 1),
      frontIndex(0),
      count(0) {}

bool RequestQueue::enqueue(const FoodRequest& value) {
    if (count == capacity) {
        return false;
    }
    int rearIndex = (frontIndex + count) % capacity;
    data[rearIndex] = value;
    ++count;
    return true;
}

bool RequestQueue::dequeue(FoodRequest& out) {
    if (count == 0) {
        return false;
    }
    out = data[frontIndex];
    frontIndex = (frontIndex + 1) % capacity;
    --count;
    return true;
}

bool RequestQueue::empty() const {
    return count == 0;
}

int RequestQueue::size() const {
    return count;
}

int RequestQueue::maxSize() const {
    return capacity;
}

const FoodRequest& RequestQueue::front() const {
    return data[frontIndex];
}

std::vector<FoodRequest> RequestQueue::toVector() const {
    std::vector<FoodRequest> result;
    result.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        result.push_back(data[(frontIndex + i) % capacity]);
    }
    return result;
}
