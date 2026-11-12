#include "SelfTests.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "BST.h"
#include "BestMatch.h"
#include "DateUtils.h"
#include "Food.h"
#include "Graph.h"
#include "MinHeap.h"
#include "Queue.h"
#include "Searching.h"
#include "SinglyLinkedList.h"
#include "Sorting.h"
#include "Stack.h"

namespace {

void checkTest(
    bool condition,
    const std::string& name,
    int& passed,
    int& failed
) {
    if (condition) {
        ++passed;
        std::cout << "  [PASS] " << name << "\n";
    } else {
        ++failed;
        std::cout << "  [FAIL] " << name << "\n";
    }
}

} // namespace

int runSelfTests() {
    std::cout << "\n===== DSA self-tests =====\n";
    int passed = 0;
    int failed = 0;

    Food apple = makeFood(1, "Apple", "Fruit", 10, "Hotel A",
                          "Street A", "2026-11-20", 0.0, 0.0);
    Food bread = makeFood(2, "Bread", "Bakery", 20, "Hotel B",
                          "Street B", "2026-11-18", 0.0, 0.0);
    Food cherry = makeFood(3, "Cherry", "Fruit", 5, "Hotel C",
                           "Street C", "2026-11-25", 0.0, 0.0);

    // Linked list
    {
        FoodList list;
        list.add(apple);
        list.add(bread);
        checkTest(list.size() == 2, "Linked list add", passed, failed);
        checkTest(list.findById(2) != nullptr,
                  "Linked list search", passed, failed);
        checkTest(list.removeById(1),
                  "Linked list remove", passed, failed);
        checkTest(list.size() == 1,
                  "Linked list size after remove", passed, failed);
    }

    // Stack
    {
        FoodStack stack;
        stack.push(apple);
        stack.push(bread);
        Food value;
        checkTest(stack.pop(value) && value.id == bread.id,
                  "Stack LIFO order", passed, failed);
        checkTest(stack.pop(value) && value.id == apple.id,
                  "Stack second pop", passed, failed);
        checkTest(stack.empty(),
                  "Stack empty after pops", passed, failed);
    }

    // Queue
    {
        RequestQueue queue(3);

        FoodRequest first;
        first.id = 1;
        first.foodId = apple.id;
        first.ngoName = "NGO A";
        first.quantity = 3;

        FoodRequest second;
        second.id = 2;
        second.foodId = bread.id;
        second.ngoName = "NGO B";
        second.quantity = 4;

        queue.enqueue(first);
        queue.enqueue(second);

        FoodRequest value;
        checkTest(queue.dequeue(value) && value.id == 1,
                  "Queue FIFO order", passed, failed);
        checkTest(queue.dequeue(value) && value.id == 2,
                  "Queue second dequeue", passed, failed);
        checkTest(queue.empty(),
                  "Queue empty after dequeues", passed, failed);
    }

    // Min-heap
    {
        FoodMinHeap heap;
        heap.push(apple);
        heap.push(cherry);
        heap.push(bread);

        Food first;
        Food second;
        checkTest(heap.pop(first) && first.id == bread.id,
                  "Min-heap returns earliest expiry", passed, failed);
        checkTest(heap.pop(second) && second.id == apple.id,
                  "Min-heap second earliest expiry", passed, failed);
    }

    // Binary search tree
    {
        FoodBST tree;
        tree.insert(apple);
        tree.insert(bread);
        tree.insert(cherry);
        checkTest(tree.find(2) != nullptr,
                  "BST search by id", passed, failed);
        checkTest(tree.find(99) == nullptr,
                  "BST missing id", passed, failed);
        checkTest(!tree.empty(),
                  "BST is not empty", passed, failed);
    }

    // Sorting
    {
        std::vector<Food> foods;
        foods.push_back(cherry);
        foods.push_back(apple);
        foods.push_back(bread);
        Sorting::mergeSortByExpiry(foods);
        checkTest(foods[0].id == bread.id
                  && foods[1].id == apple.id
                  && foods[2].id == cherry.id,
                  "Merge sort by expiry", passed, failed);

        foods.clear();
        foods.push_back(cherry);
        foods.push_back(bread);
        foods.push_back(apple);
        Sorting::quickSortByName(foods);
        checkTest(foods[0].name == "Apple"
                  && foods[1].name == "Bread"
                  && foods[2].name == "Cherry",
                  "Quick sort by name", passed, failed);

        foods.clear();
        foods.push_back(bread);
        foods.push_back(apple);
        foods.push_back(cherry);
        Sorting::insertionSortByQuantity(foods);
        checkTest(foods[0].id == cherry.id
                  && foods[1].id == apple.id
                  && foods[2].id == bread.id,
                  "Insertion sort by quantity", passed, failed);
    }

    // Searching
    {
        std::vector<Food> foods;
        foods.push_back(cherry);
        foods.push_back(apple);
        foods.push_back(bread);
        Sorting::mergeSortById(foods);

        int position = Searching::binarySearchById(foods, 2);
        checkTest(position >= 0 && foods[position].id == 2,
                  "Binary search finds id", passed, failed);
        checkTest(Searching::binarySearchById(foods, 99) == -1,
                  "Binary search missing id", passed, failed);

        std::vector<Food> matches =
            Searching::linearSearchByName(foods, "bre");
        checkTest(matches.size() == 1 && matches[0].id == bread.id,
                  "Linear search by name", passed, failed);
    }

    // Graph
    {
        FoodGraph graph(4);
        graph.addEdge(0, 1);
        graph.addEdge(1, 2);

        std::vector<int> bfsOrder = graph.bfs(0);
        std::vector<int> dfsOrder = graph.dfs(0);

        checkTest(std::find(bfsOrder.begin(), bfsOrder.end(), 2)
                      != bfsOrder.end(),
                  "BFS reaches connected node", passed, failed);
        checkTest(std::find(dfsOrder.begin(), dfsOrder.end(), 2)
                      != dfsOrder.end(),
                  "DFS reaches connected node", passed, failed);
        checkTest(std::find(bfsOrder.begin(), bfsOrder.end(), 3)
                      == bfsOrder.end(),
                  "BFS does not reach disconnected node", passed, failed);
    }

    // Hash map
    {
        std::unordered_map<std::string, int> categoryTotals;
        categoryTotals[apple.category] += apple.quantity;
        categoryTotals[bread.category] += bread.quantity;
        checkTest(categoryTotals["Fruit"] == 10,
                  "Hash map category total", passed, failed);
        checkTest(categoryTotals["Bakery"] == 20,
                  "Hash map second category", passed, failed);
    }

    // Date helpers
    {
        checkTest(DateUtils::addDays("2026-11-12", 1) == "2026-11-13",
                  "Date helper adds days", passed, failed);
        checkTest(DateUtils::daysBetween("2026-11-12", "2026-11-15") == 3,
                  "Date helper counts days", passed, failed);
    }

    // Best match: distance + urgency + quantity + category
    {
        const std::string today = "2026-11-12";
        const double ngoLat = 28.6139;
        const double ngoLng = 77.2090;

        Food nearUrgent = makeFood(
            201, "Near Urgent", "Cooked Meals", 30, "Hotel N", "Near",
            today, ngoLat, ngoLng);
        Food nearLater = makeFood(
            202, "Near Later", "Cooked Meals", 30, "Hotel N", "Near",
            "2026-11-20", ngoLat, ngoLng);
        Food farUrgent = makeFood(
            203, "Far Urgent", "Cooked Meals", 30, "Hotel F", "Far",
            today, 28.7000, 77.3000);
        Food expired = makeFood(
            204, "Expired Food", "Cooked Meals", 30, "Hotel E", "Near",
            "2026-11-11", ngoLat, ngoLng);
        Food smallQuantity = makeFood(
            205, "Small Quantity", "Cooked Meals", 5, "Hotel S", "Near",
            today, ngoLat, ngoLng);
        Food otherCategory = makeFood(
            206, "Other Category", "Bakery", 30, "Hotel O", "Near",
            today, ngoLat, ngoLng);
        Food farLater = makeFood(
            207, "Far Later", "Cooked Meals", 30, "Hotel F", "Far",
            "2026-11-20", 28.7000, 77.3000);

        std::vector<Food> all;
        all.push_back(nearUrgent);
        all.push_back(nearLater);
        all.push_back(farUrgent);
        all.push_back(expired);
        all.push_back(smallQuantity);
        all.push_back(otherCategory);
        all.push_back(farLater);

        std::vector<MatchResult> results =
            BestMatch::rank(all, ngoLat, ngoLng, 20, "", today, 10);

        checkTest(results.size() == 6,
                  "Best match excludes expired food", passed, failed);
        checkTest(!results.empty() && results[0].food.id == nearUrgent.id,
                  "Best match ranks best overall first", passed, failed);

        std::vector<Food> distancePair;
        distancePair.push_back(farLater);
        distancePair.push_back(nearLater);
        std::vector<MatchResult> distanceResults =
            BestMatch::rank(distancePair, ngoLat, ngoLng, 20, "", today, 10);
        checkTest(distanceResults.size() == 2
                      && distanceResults[0].food.id == nearLater.id,
                  "Best match prefers closer food", passed, failed);

        std::vector<Food> urgencyPair;
        urgencyPair.push_back(nearLater);
        urgencyPair.push_back(nearUrgent);
        std::vector<MatchResult> urgencyResults =
            BestMatch::rank(urgencyPair, ngoLat, ngoLng, 20, "", today, 10);
        checkTest(urgencyResults.size() == 2
                      && urgencyResults[0].food.id == nearUrgent.id,
                  "Best match prefers urgent food", passed, failed);

        std::vector<Food> quantityPair;
        quantityPair.push_back(smallQuantity);
        quantityPair.push_back(nearUrgent);
        std::vector<MatchResult> quantityResults =
            BestMatch::rank(quantityPair, ngoLat, ngoLng, 20, "", today, 10);
        checkTest(quantityResults.size() == 2
                      && quantityResults[0].food.id == nearUrgent.id,
                  "Best match prefers enough quantity", passed, failed);

        std::vector<Food> categoryPair;
        categoryPair.push_back(otherCategory);
        categoryPair.push_back(nearUrgent);
        std::vector<MatchResult> categoryResults = BestMatch::rank(
            categoryPair, ngoLat, ngoLng, 20, "Cooked Meals", today, 10);
        checkTest(categoryResults.size() == 2
                      && categoryResults[0].food.id == nearUrgent.id,
                  "Best match prefers category match", passed, failed);

        std::vector<MatchResult> limited =
            BestMatch::rank(all, ngoLat, ngoLng, 20, "", today, 2);
        checkTest(limited.size() == 2,
                  "Best match respects result limit", passed, failed);

        bool scoresInRange = true;
        for (const MatchResult& result : results) {
            if (result.totalScore < 0.0 || result.totalScore > 100.0) {
                scoresInRange = false;
            }
        }
        checkTest(scoresInRange,
                  "Best match score stays in 0..100", passed, failed);

        (void)farUrgent; // scored through the `all` vector above
    }

    std::cout << "\n  Result: " << passed << " passed, "
              << failed << " failed.\n";
    return failed;
}
