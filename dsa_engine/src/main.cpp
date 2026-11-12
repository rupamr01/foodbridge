#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "BST.h"
#include "BestMatch.h"
#include "DateUtils.h"
#include "Food.h"
#include "FoodStore.h"
#include "Graph.h"
#include "MinHeap.h"
#include "Queue.h"
#include "Searching.h"
#include "SeedData.h"
#include "SelfTests.h"
#include "SinglyLinkedList.h"
#include "Sorting.h"
#include "Stack.h"

namespace {

std::string trim(const std::string& text) {
    std::size_t start = text.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    std::size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(start, end - start + 1);
}

std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string value;
    if (!std::getline(std::cin, value)) {
        return "";
    }
    return trim(value);
}

int readInt(const std::string& prompt) {
    while (true) {
        std::string text = readLine(prompt);
        if (text.empty() && std::cin.eof()) {
            return 0;
        }

        try {
            std::size_t used = 0;
            int value = std::stoi(text, &used);
            if (used == text.size()) {
                return value;
            }
        } catch (...) {
            // Fall through to the error message.
        }
        std::cout << "  Please enter a whole number.\n";
    }
}

double readDouble(const std::string& prompt) {
    while (true) {
        std::string text = readLine(prompt);
        if (text.empty() && std::cin.eof()) {
            return 0.0;
        }

        try {
            std::size_t used = 0;
            double value = std::stod(text, &used);
            if (used == text.size()) {
                return value;
            }
        } catch (...) {
            // Fall through to the error message.
        }
        std::cout << "  Please enter a number.\n";
    }
}

void printFoodDetails(const Food& food) {
    std::cout << "  ID: " << food.id << "\n"
              << "  Name: " << food.name << "\n"
              << "  Category: " << food.category << "\n"
              << "  Quantity: " << food.quantity << "\n"
              << "  Donor: " << food.donor << "\n"
              << "  Pickup: " << food.pickup << "\n"
              << "  Best before: " << food.bestBefore << "\n"
              << "  Coordinates: " << food.latitude << ", "
              << food.longitude << "\n";
}

void printFoodTable(const std::vector<Food>& foods) {
    if (foods.empty()) {
        std::cout << "  (no food listings)\n";
        return;
    }

    std::cout << std::left
              << std::setw(6) << "ID"
              << std::setw(22) << "Name"
              << std::setw(18) << "Category"
              << std::setw(6) << "Qty"
              << std::setw(14) << "Best before"
              << "Donor\n";
    std::cout << std::string(80, '-') << "\n";

    for (const Food& food : foods) {
        std::cout << std::left
                  << std::setw(6) << food.id
                  << std::setw(22) << food.name.substr(0, 20)
                  << std::setw(18) << food.category.substr(0, 16)
                  << std::setw(6) << food.quantity
                  << std::setw(14) << food.bestBefore
                  << food.donor << "\n";
    }
}

void showExpiringSoon(const std::vector<Food>& foods, int limit) {
    FoodMinHeap heap;
    for (const Food& food : foods) {
        heap.push(food);
    }

    if (heap.empty()) {
        std::cout << "  (no food listings)\n";
        return;
    }

    std::cout << "  Earliest best-before dates first (min-heap):\n";
    for (int i = 0; i < limit && !heap.empty(); ++i) {
        Food food;
        heap.pop(food);
        std::cout << "  " << (i + 1) << ". [" << food.id << "] "
                  << food.name << " | best before " << food.bestBefore
                  << " | qty " << food.quantity << "\n";
    }
}

void printTotals(
    const std::string& title,
    const std::unordered_map<std::string, int>& totals
) {
    std::cout << "  " << title << " (hash map):\n";
    if (totals.empty()) {
        std::cout << "    (none)\n";
        return;
    }

    std::vector<std::pair<int, std::string> > rows;
    for (std::unordered_map<std::string, int>::const_iterator it =
             totals.begin(); it != totals.end(); ++it) {
        rows.push_back(std::make_pair(it->second, it->first));
    }

    std::sort(rows.begin(), rows.end(),
        [](const std::pair<int, std::string>& a,
           const std::pair<int, std::string>& b) {
            if (a.first != b.first) {
                return a.first > b.first;
            }
            return a.second < b.second;
        });

    for (const std::pair<int, std::string>& row : rows) {
        std::cout << "    " << row.second << ": " << row.first << "\n";
    }
}

void showAnalytics(const std::vector<Food>& foods) {
    std::unordered_map<std::string, int> categoryTotals;
    std::unordered_map<std::string, int> donorTotals;
    int totalQuantity = 0;

    for (const Food& food : foods) {
        categoryTotals[food.category] += food.quantity;
        donorTotals[food.donor] += food.quantity;
        totalQuantity += food.quantity;
    }

    std::cout << "  Total quantity available: " << totalQuantity << "\n";
    printTotals("By category", categoryTotals);
    printTotals("By donor", donorTotals);
}

void showGraph(const std::vector<Food>& foods, bool breadthFirst) {
    if (foods.empty()) {
        std::cout << "  (no food listings)\n";
        return;
    }

    const double maxDistanceKm = 1.5;
    FoodGraph graph(static_cast<int>(foods.size()));
    int edgeCount = 0;

    for (int i = 0; i < static_cast<int>(foods.size()); ++i) {
        for (int j = i + 1; j < static_cast<int>(foods.size()); ++j) {
            double distance = distanceKm(
                foods[i].latitude, foods[i].longitude,
                foods[j].latitude, foods[j].longitude);
            if (distance <= maxDistanceKm) {
                graph.addEdge(i, j);
                ++edgeCount;
            }
        }
    }

    std::cout << "  Graph built: " << foods.size() << " nodes, "
              << edgeCount << " edges (pickup points within "
              << maxDistanceKm << " km).\n";

    int startId = readInt("  Start food ID: ");
    int startIndex = -1;

    for (int i = 0; i < static_cast<int>(foods.size()); ++i) {
        if (foods[i].id == startId) {
            startIndex = i;
            break;
        }
    }

    if (startIndex < 0) {
        std::cout << "  No food listing has ID " << startId << ".\n";
        return;
    }

    std::vector<int> order = breadthFirst
        ? graph.bfs(startIndex)
        : graph.dfs(startIndex);

    std::cout << "  " << (breadthFirst ? "BFS" : "DFS")
              << " visit order:\n";
    for (std::size_t i = 0; i < order.size(); ++i) {
        const Food& food = foods[order[i]];
        std::cout << "    [" << food.id << "] " << food.name
                  << " - " << food.pickup << "\n";
    }
}

void printMenu() {
    std::cout << "\n===== FoodBridge DSA Lab =====\n"
              << "  1. Show all food listings        (linked list)\n"
              << "  2. Add a food listing            (linked list + hash map + BST)\n"
              << "  3. Remove a food listing         (linked list + hash map)\n"
              << "  4. Search by ID                  (hash map)\n"
              << "  5. Search by ID                  (BST)\n"
              << "  6. Search by ID                  (binary search)\n"
              << "  7. Search by name                (linear search)\n"
              << "  8. Sort by expiry date           (merge sort)\n"
              << "  9. Sort by quantity              (insertion sort)\n"
              << " 10. Sort by name                  (quick sort)\n"
              << " 11. Show expiring soon            (min-heap)\n"
              << " 12. Add an NGO request            (queue)\n"
              << " 13. Process the next request      (queue)\n"
              << " 14. Show pending requests         (queue)\n"
              << " 15. Undo the last added listing   (stack)\n"
              << " 16. Category and donor totals     (hash map)\n"
              << " 17. Show BST in-order traversal   (BST)\n"
              << " 18. BFS pickup network            (graph)\n"
              << " 19. DFS pickup network            (graph)\n"
              << " 20. Run all DSA self-tests\n"
              << " 21. Best match for an NGO         (score + merge sort)\n"
              << "  0. Exit\n";
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc > 1 && std::string(argv[1]) == "--selftest") {
        return runSelfTests() == 0 ? 0 : 1;
    }

    FoodList foodList;
    std::unordered_map<int, Food> foodIndex;
    FoodBST foodTree;
    FoodStack undoStack;
    RequestQueue requestQueue(100);

    int nextFoodId = 109;
    int nextRequestId = 1;

    SeedData::load(foodList, foodIndex, foodTree);

    std::cout << "FoodBridge DSA Lab\n"
              << "A small console program for testing data structures and algorithms.\n"
              << "No Flask, MySQL, or internet connection is required.\n";

    while (true) {
        printMenu();
        int choice = readInt("Choose an option: ");

        if (choice == 0) {
            std::cout << "Goodbye.\n";
            return 0;
        }

        switch (choice) {
        case 1:
            std::cout << "\n  --- All food listings (linked list traversal) ---\n";
            foodList.display();
            break;

        case 2: {
            std::cout << "\n  --- Add food listing ---\n";
            std::string name = readLine("  Food name: ");
            std::string category = readLine("  Category: ");
            int quantity = readInt("  Quantity: ");
            std::string donor = readLine("  Donor/hotel: ");
            std::string pickup = readLine("  Pickup address: ");
            std::string bestBefore = readLine("  Best before (YYYY-MM-DD): ");
            double latitude = readDouble("  Latitude: ");
            double longitude = readDouble("  Longitude: ");

            if (name.empty() || category.empty()
                    || donor.empty() || pickup.empty()) {
                std::cout << "  Name, category, donor, and pickup are required.\n";
                break;
            }
            if (quantity <= 0) {
                std::cout << "  Quantity must be greater than zero.\n";
                break;
            }
            if (!DateUtils::isIsoDate(bestBefore)) {
                std::cout << "  Use the date format YYYY-MM-DD.\n";
                break;
            }
            if (latitude < -90.0 || latitude > 90.0
                    || longitude < -180.0 || longitude > 180.0) {
                std::cout << "  Latitude must be -90..90 and longitude -180..180.\n";
                break;
            }

            Food food = makeFood(nextFoodId, name, category, quantity,
                                 donor, pickup, bestBefore, latitude, longitude);
            addFoodToStore(foodList, foodIndex, foodTree, food);
            undoStack.push(food);
            ++nextFoodId;

            std::cout << "  Added food with ID " << food.id << ".\n";
            break;
        }

        case 3: {
            int id = readInt("  Food ID to remove: ");
            if (removeFoodFromStore(foodList, foodIndex, foodTree, id)) {
                std::cout << "  Removed food ID " << id << ".\n";
            } else {
                std::cout << "  Food ID " << id << " was not found.\n";
            }
            break;
        }

        case 4: {
            int id = readInt("  Food ID: ");
            std::unordered_map<int, Food>::const_iterator it =
                foodIndex.find(id);
            if (it == foodIndex.end()) {
                std::cout << "  Food ID " << id << " was not found.\n";
            } else {
                std::cout << "  Found instantly with the hash map:\n";
                printFoodDetails(it->second);
            }
            break;
        }

        case 5: {
            int id = readInt("  Food ID: ");
            const Food* food = foodTree.find(id);
            if (food == nullptr) {
                std::cout << "  Food ID " << id << " was not found.\n";
            } else {
                std::cout << "  Found with the binary search tree:\n";
                printFoodDetails(*food);
            }
            break;
        }

        case 6: {
            int id = readInt("  Food ID: ");
            std::vector<Food> foods = foodList.toVector();
            Sorting::mergeSortById(foods);
            int position = Searching::binarySearchById(foods, id);

            if (position < 0) {
                std::cout << "  Food ID " << id << " was not found.\n";
            } else {
                std::cout << "  Found at sorted index " << position
                          << " using binary search:\n";
                printFoodDetails(foods[position]);
            }
            break;
        }

        case 7: {
            std::string query = readLine("  Name contains: ");
            std::vector<Food> matches = Searching::linearSearchByName(
                foodList.toVector(), query);

            if (matches.empty()) {
                std::cout << "  No food name matched.\n";
            } else {
                std::cout << "  Linear search found " << matches.size()
                          << " match(es):\n";
                printFoodTable(matches);
            }
            break;
        }

        case 8: {
            std::vector<Food> foods = foodList.toVector();
            Sorting::mergeSortByExpiry(foods);
            std::cout << "\n  --- Sorted by earliest best-before date"
                      << " (merge sort) ---\n";
            printFoodTable(foods);
            break;
        }

        case 9: {
            std::vector<Food> foods = foodList.toVector();
            Sorting::insertionSortByQuantity(foods);
            std::cout << "\n  --- Sorted by smallest quantity"
                      << " (insertion sort) ---\n";
            printFoodTable(foods);
            break;
        }

        case 10: {
            std::vector<Food> foods = foodList.toVector();
            Sorting::quickSortByName(foods);
            std::cout << "\n  --- Sorted by food name (quick sort) ---\n";
            printFoodTable(foods);
            break;
        }

        case 11:
            std::cout << "\n  --- Expiring soon (min-heap) ---\n";
            showExpiringSoon(foodList.toVector(), 5);
            break;

        case 12: {
            std::cout << "\n  --- Add NGO request ---\n";
            int foodId = readInt("  Food ID: ");
            std::unordered_map<int, Food>::const_iterator food =
                foodIndex.find(foodId);
            if (food == foodIndex.end()) {
                std::cout << "  Food ID " << foodId << " was not found.\n";
                break;
            }

            std::string ngoName = readLine("  NGO name: ");
            int quantity = readInt("  Requested quantity: ");

            if (ngoName.empty()) {
                std::cout << "  NGO name is required.\n";
                break;
            }
            if (quantity <= 0) {
                std::cout << "  Quantity must be greater than zero.\n";
                break;
            }

            FoodRequest request;
            request.id = nextRequestId;
            request.foodId = foodId;
            request.ngoName = ngoName;
            request.quantity = quantity;

            if (requestQueue.enqueue(request)) {
                std::cout << "  Request #" << request.id
                          << " added to the queue for "
                          << food->second.name << ".\n";
                ++nextRequestId;
            } else {
                std::cout << "  The request queue is full.\n";
            }
            break;
        }

        case 13: {
            FoodRequest request;
            if (!requestQueue.dequeue(request)) {
                std::cout << "  No pending requests.\n";
                break;
            }

            std::unordered_map<int, Food>::const_iterator food =
                foodIndex.find(request.foodId);
            std::string foodName = food == foodIndex.end()
                ? "(removed listing)"
                : food->second.name;

            std::cout << "  Processed request #" << request.id
                      << " from " << request.ngoName
                      << " for " << request.quantity << " x "
                      << foodName << ".\n";
            break;
        }

        case 14: {
            std::vector<FoodRequest> pending = requestQueue.toVector();
            if (pending.empty()) {
                std::cout << "  No pending requests.\n";
                break;
            }

            std::cout << "  Pending requests (first in, first out):\n";
            for (std::size_t i = 0; i < pending.size(); ++i) {
                const FoodRequest& request = pending[i];
                std::unordered_map<int, Food>::const_iterator food =
                    foodIndex.find(request.foodId);
                std::string foodName = food == foodIndex.end()
                    ? "(removed listing)"
                    : food->second.name;

                std::cout << "    " << (i + 1) << ". #" << request.id
                          << " | " << request.ngoName
                          << " | " << request.quantity << " x "
                          << foodName << "\n";
            }
            break;
        }

        case 15: {
            Food last;
            if (!undoStack.pop(last)) {
                std::cout << "  Nothing to undo.\n";
                break;
            }

            if (removeFoodFromStore(foodList, foodIndex, foodTree, last.id)) {
                std::cout << "  Undid adding [" << last.id << "] "
                          << last.name << ".\n";
            } else {
                std::cout << "  That listing was already removed.\n";
            }
            break;
        }

        case 16:
            std::cout << "\n  --- Category and donor totals (hash map) ---\n";
            showAnalytics(foodList.toVector());
            break;

        case 17:
            std::cout << "\n  --- BST in-order traversal (sorted by ID) ---\n";
            if (foodTree.empty()) {
                std::cout << "  (no food listings)\n";
            } else {
                foodTree.inorder();
            }
            break;

        case 18:
            std::cout << "\n  --- BFS on the pickup network ---\n";
            showGraph(foodList.toVector(), true);
            break;

        case 19:
            std::cout << "\n  --- DFS on the pickup network ---\n";
            showGraph(foodList.toVector(), false);
            break;

        case 20:
            runSelfTests();
            break;

        case 21: {
            std::cout << "\n  --- Best match for an NGO ---\n";
            std::cout << "  Location is the NGO's latitude/longitude.\n";

            double latitude = readDouble("  NGO latitude: ");
            double longitude = readDouble("  NGO longitude: ");
            int needed = readInt("  Quantity needed: ");
            std::string category = readLine(
                "  Preferred category (blank for any): ");
            int limit = readInt("  Results to show (0 for 5): ");

            if (needed <= 0) {
                std::cout << "  Quantity must be greater than zero.\n";
                break;
            }
            if (latitude < -90.0 || latitude > 90.0
                    || longitude < -180.0 || longitude > 180.0) {
                std::cout << "  Latitude must be -90..90"
                          << " and longitude -180..180.\n";
                break;
            }
            if (limit <= 0) {
                limit = 5;
            }

            std::vector<MatchResult> matches = BestMatch::rank(
                foodList.toVector(),
                latitude,
                longitude,
                needed,
                category,
                DateUtils::todayIso(),
                limit
            );

            BestMatch::display(matches);
            break;
        }

        default:
            std::cout << "  Choose a number from the menu.\n";
            break;
        }
    }
}
