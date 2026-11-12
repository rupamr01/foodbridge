#pragma once

#include <string>

#include "Food.h"

// One scored listing returned by the best-match ranking.
// Each component score is in the range 0.0 to 1.0.
// totalScore is in the range 0 to 100.
struct MatchResult {
    Food food;
    double distance;       // straight-line distance in km
    int daysLeft;          // days until bestBefore (0 = today)
    double distanceScore;
    double urgencyScore;
    double quantityScore;
    double categoryScore;
    double totalScore;
};
