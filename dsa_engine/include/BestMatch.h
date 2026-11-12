#pragma once

#include <string>
#include <vector>

#include "Food.h"
#include "MatchResult.h"

// Balanced, rule-based "best match" for an NGO.
//
// The score is intentionally simple arithmetic:
//   45% distance + 30% expiry urgency + 15% quantity fit + 10% category
//
// No machine learning or greedy algorithm is used. After scoring, the results
// are ordered with the hand-written merge sort from Sorting.cpp.
namespace BestMatch {

// Weights add up to 1.0.
constexpr double WEIGHT_DISTANCE = 0.45;
constexpr double WEIGHT_URGENCY = 0.30;
constexpr double WEIGHT_QUANTITY = 0.15;
constexpr double WEIGHT_CATEGORY = 0.10;

// A listing farther than this gets a distance score of 0.
constexpr double MAX_DISTANCE_KM = 20.0;

// Food that expires within this many days is treated as urgent.
constexpr int URGENCY_WINDOW_DAYS = 14;

// Score one listing. Expired food is still scored here; rank() removes it.
MatchResult score(
    const Food& food,
    double ngoLatitude,
    double ngoLongitude,
    int requestedQuantity,
    const std::string& preferredCategory,
    const std::string& todayIso
);

// Score every non-expired listing, sort highest score first, and return at
// most `limit` results. A limit of 0 or less means the default of 5.
std::vector<MatchResult> rank(
    const std::vector<Food>& foods,
    double ngoLatitude,
    double ngoLongitude,
    int requestedQuantity,
    const std::string& preferredCategory,
    const std::string& todayIso,
    int limit
);

// Print a ranked result table for the console menu.
void display(const std::vector<MatchResult>& results);

} // namespace BestMatch
