#include "SeedData.h"

#include <string>

#include "DateUtils.h"
#include "FoodStore.h"

namespace SeedData {

void load(
    FoodList& foods,
    std::unordered_map<int, Food>& index,
    FoodBST& tree
) {
    const std::string today = DateUtils::todayIso();

    addFoodToStore(foods, index, tree, makeFood(
        101, "Fresh Bread", "Bakery", 25, "Sunrise Hotel",
        "Green Street", DateUtils::addDays(today, 1), 28.6139, 77.2090));

    addFoodToStore(foods, index, tree, makeFood(
        102, "Cooked Rice", "Cooked Meals", 40, "Grand Palace",
        "Market Road", DateUtils::addDays(today, 2), 28.6200, 77.2150));

    addFoodToStore(foods, index, tree, makeFood(
        103, "Milk Packs", "Dairy", 18, "City Hotel",
        "Station Road", today, 28.6100, 77.2000));

    addFoodToStore(foods, index, tree, makeFood(
        104, "Mixed Vegetables", "Fresh Produce", 30, "Green Leaf Hotel",
        "Park Avenue", DateUtils::addDays(today, 4), 28.6300, 77.2200));

    addFoodToStore(foods, index, tree, makeFood(
        105, "Bread Rolls", "Bakery", 12, "Sunrise Hotel",
        "Green Street", DateUtils::addDays(today, 1), 28.6139, 77.2090));

    addFoodToStore(foods, index, tree, makeFood(
        106, "Packaged Snacks", "Packaged Food", 50, "Budget Inn",
        "Ring Road", DateUtils::addDays(today, 19), 28.6500, 77.2400));

    addFoodToStore(foods, index, tree, makeFood(
        107, "Curd Cups", "Dairy", 22, "City Hotel",
        "Station Road", DateUtils::addDays(today, 3), 28.6100, 77.2000));

    addFoodToStore(foods, index, tree, makeFood(
        108, "Fruit Boxes", "Fresh Produce", 16, "Grand Palace",
        "Market Road", DateUtils::addDays(today, 5), 28.6200, 77.2150));
}

} // namespace SeedData
