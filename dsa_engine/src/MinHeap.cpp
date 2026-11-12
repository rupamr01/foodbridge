#include "MinHeap.h"

#include <utility>

bool FoodExpiryLess::operator()(const Food& a, const Food& b) const {
    if (a.bestBefore != b.bestBefore) {
        return a.bestBefore < b.bestBefore;
    }
    return a.id < b.id;
}

void FoodMinHeap::siftUp(int index) {
    while (index > 0) {
        int parent = (index - 1) / 2;
        if (compare(heap[index], heap[parent])) {
            std::swap(heap[index], heap[parent]);
            index = parent;
        } else {
            break;
        }
    }
}

void FoodMinHeap::siftDown(int index) {
    int size = static_cast<int>(heap.size());
    while (true) {
        int left = index * 2 + 1;
        int right = index * 2 + 2;
        int smallest = index;

        if (left < size && compare(heap[left], heap[smallest])) {
            smallest = left;
        }
        if (right < size && compare(heap[right], heap[smallest])) {
            smallest = right;
        }
        if (smallest == index) {
            break;
        }

        std::swap(heap[index], heap[smallest]);
        index = smallest;
    }
}

void FoodMinHeap::push(const Food& value) {
    heap.push_back(value);
    siftUp(static_cast<int>(heap.size()) - 1);
}

bool FoodMinHeap::pop(Food& out) {
    if (heap.empty()) {
        return false;
    }

    out = heap.front();
    heap.front() = heap.back();
    heap.pop_back();

    if (!heap.empty()) {
        siftDown(0);
    }
    return true;
}

bool FoodMinHeap::empty() const {
    return heap.empty();
}

std::size_t FoodMinHeap::size() const {
    return heap.size();
}

const Food& FoodMinHeap::top() const {
    return heap.front();
}
