#pragma once

#include <string>

// A food listing in the standalone DSA lab.
// The real Flask app stores these in MySQL; here we keep them in memory so
// every data structure can be demonstrated without a database.
struct Food {
    int id;
    std::string name;
    std::string category;
    int quantity;
    std::string donor;
    std::string pickup;
    std::string bestBefore; // YYYY-MM-DD, so string comparison is date order
    double latitude;
    double longitude;
};

// An NGO request waiting in the queue.
struct FoodRequest {
    int id;
    int foodId;
    std::string ngoName;
    int quantity;
};

inline Food makeFood(
    int id,
    const std::string& name,
    const std::string& category,
    int quantity,
    const std::string& donor,
    const std::string& pickup,
    const std::string& bestBefore,
    double latitude,
    double longitude
) {
    Food food;
    food.id = id;
    food.name = name;
    food.category = category;
    food.quantity = quantity;
    food.donor = donor;
    food.pickup = pickup;
    food.bestBefore = bestBefore;
    food.latitude = latitude;
    food.longitude = longitude;
    return food;
}
