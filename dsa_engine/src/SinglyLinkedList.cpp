#include "SinglyLinkedList.h"

#include <iomanip>
#include <iostream>
#include <string>

FoodList::FoodList() : head(nullptr), count(0) {}

FoodList::~FoodList() {
    clear();
}

void FoodList::add(const Food& food) {
    Node* node = new Node(food);
    if (head == nullptr) {
        head = node;
    } else {
        Node* current = head;
        while (current->next != nullptr) {
            current = current->next;
        }
        current->next = node;
    }
    ++count;
}

bool FoodList::removeById(int id) {
    Node* current = head;
    Node* previous = nullptr;

    while (current != nullptr && current->data.id != id) {
        previous = current;
        current = current->next;
    }

    if (current == nullptr) {
        return false;
    }

    if (previous == nullptr) {
        head = current->next;
    } else {
        previous->next = current->next;
    }

    delete current;
    --count;
    return true;
}

const Food* FoodList::findById(int id) const {
    Node* current = head;
    while (current != nullptr) {
        if (current->data.id == id) {
            return &current->data;
        }
        current = current->next;
    }
    return nullptr;
}

std::vector<Food> FoodList::toVector() const {
    std::vector<Food> result;
    result.reserve(static_cast<std::size_t>(count));

    Node* current = head;
    while (current != nullptr) {
        result.push_back(current->data);
        current = current->next;
    }
    return result;
}

int FoodList::size() const {
    return count;
}

bool FoodList::empty() const {
    return count == 0;
}

void FoodList::display() const {
    if (head == nullptr) {
        std::cout << "  (no food listings yet)\n";
        return;
    }

    std::cout << std::left
              << std::setw(6) << "ID"
              << std::setw(22) << "Name"
              << std::setw(18) << "Category"
              << std::setw(6) << "Qty"
              << std::setw(14) << "Best before"
              << std::setw(18) << "Donor"
              << "Pickup\n";
    std::cout << std::string(100, '-') << "\n";

    Node* current = head;
    while (current != nullptr) {
        const Food& food = current->data;
        std::cout << std::left
                  << std::setw(6) << food.id
                  << std::setw(22) << food.name.substr(0, 20)
                  << std::setw(18) << food.category.substr(0, 16)
                  << std::setw(6) << food.quantity
                  << std::setw(14) << food.bestBefore
                  << std::setw(18) << food.donor.substr(0, 16)
                  << food.pickup << "\n";
        current = current->next;
    }
}

void FoodList::clear() {
    Node* current = head;
    while (current != nullptr) {
        Node* next = current->next;
        delete current;
        current = next;
    }
    head = nullptr;
    count = 0;
}
