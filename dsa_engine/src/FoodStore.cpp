#include "FoodStore.h"

void addFoodToStore(
    FoodList& foods,
    std::unordered_map<int, Food>& index,
    FoodBST& tree,
    const Food& food
) {
    foods.add(food);
    index[food.id] = food;
    tree.insert(food);
}

bool removeFoodFromStore(
    FoodList& foods,
    std::unordered_map<int, Food>& index,
    FoodBST& tree,
    int id
) {
    if (!foods.removeById(id)) {
        return false;
    }
    index.erase(id);
    tree.rebuildFrom(foods.toVector());
    return true;
}
