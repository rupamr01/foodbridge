#include "BestMatch.h"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "DateUtils.h"
#include "Graph.h"
#include "Searching.h"
#include "Sorting.h"

namespace {

double clamp01(double value) {
    if (value < 0.0) {
        return 0.0;
    }
    if (value > 1.0) {
        return 1.0;
    }
    return value;
}

std::string formatNumber(double value, int precision) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(precision) << value;
    return out.str();
}

} // namespace

namespace BestMatch {

MatchResult score(
    const Food& food,
    double ngoLatitude,
    double ngoLongitude,
    int requestedQuantity,
    const std::string& preferredCategory,
    const std::string& todayIso
) {
    MatchResult result;
    result.food = food;
    result.daysLeft = DateUtils::daysBetween(todayIso, food.bestBefore);
    result.distance = distanceKm(
        ngoLatitude,
        ngoLongitude,
        food.latitude,
        food.longitude
    );

    // 1.0 when the pickup point is at the NGO, falling to 0.0 at 20 km.
    result.distanceScore = clamp01(1.0 - result.distance / MAX_DISTANCE_KM);

    // Food expiring today scores 1.0. At 14 or more days it scores 0.0.
    if (result.daysLeft <= 0) {
        result.urgencyScore = 1.0;
    } else if (result.daysLeft >= URGENCY_WINDOW_DAYS) {
        result.urgencyScore = 0.0;
    } else {
        result.urgencyScore = clamp01(
            1.0 - static_cast<double>(result.daysLeft) / URGENCY_WINDOW_DAYS
        );
    }

    // Enough food for the request is a perfect fit. A short quantity gets a
    // proportional score (for example 5 of 20 requested = 0.25).
    if (requestedQuantity <= 0) {
        result.quantityScore = 1.0;
    } else {
        result.quantityScore = clamp01(
            static_cast<double>(food.quantity) / requestedQuantity
        );
    }

    // A blank category means "any category", so every listing gets the full
    // category points and the weight does not change the order.
    if (preferredCategory.empty()) {
        result.categoryScore = 1.0;
    } else {
        result.categoryScore =
            Searching::toLower(food.category)
                == Searching::toLower(preferredCategory)
            ? 1.0
            : 0.0;
    }

    result.totalScore = 100.0 * (
        WEIGHT_DISTANCE * result.distanceScore
        + WEIGHT_URGENCY * result.urgencyScore
        + WEIGHT_QUANTITY * result.quantityScore
        + WEIGHT_CATEGORY * result.categoryScore
    );

    return result;
}

std::vector<MatchResult> rank(
    const std::vector<Food>& foods,
    double ngoLatitude,
    double ngoLongitude,
    int requestedQuantity,
    const std::string& preferredCategory,
    const std::string& todayIso,
    int limit
) {
    std::vector<MatchResult> results;

    for (const Food& food : foods) {
        int daysLeft = DateUtils::daysBetween(todayIso, food.bestBefore);
        if (daysLeft < 0 || food.quantity <= 0) {
            continue; // expired or unavailable
        }

        results.push_back(score(
            food,
            ngoLatitude,
            ngoLongitude,
            requestedQuantity,
            preferredCategory,
            todayIso
        ));
    }

    Sorting::mergeSortByMatchScore(results);

    int safeLimit = limit > 0 ? limit : 5;
    if (static_cast<int>(results.size()) > safeLimit) {
        results.resize(static_cast<std::size_t>(safeLimit));
    }
    return results;
}

void display(const std::vector<MatchResult>& results) {
    if (results.empty()) {
        std::cout << "  (no available food matched)\n";
        return;
    }

    std::cout << "  Score = 45% distance + 30% urgency"
              << " + 15% quantity + 10% category\n\n";
    std::cout << std::left
              << std::setw(5) << "Rank"
              << std::setw(6) << "ID"
              << std::setw(22) << "Food"
              << std::setw(12) << "Distance"
              << std::setw(11) << "Expires"
              << std::setw(7) << "Qty"
              << "Score\n";
    std::cout << std::string(72, '-') << "\n";

    for (std::size_t i = 0; i < results.size(); ++i) {
        const MatchResult& result = results[i];

        std::ostringstream expiry;
        if (result.daysLeft == 0) {
            expiry << "today";
        } else {
            expiry << "in " << result.daysLeft << "d";
        }

        std::cout << std::left
                  << std::setw(5) << (i + 1)
                  << std::setw(6) << result.food.id
                  << std::setw(22) << result.food.name.substr(0, 20)
                  << std::setw(12) << (formatNumber(result.distance, 1) + " km")
                  << std::setw(11) << expiry.str()
                  << std::setw(7) << result.food.quantity
                  << formatNumber(result.totalScore, 1) << "\n";
        std::cout << "       " << result.food.category
                  << " | " << result.food.donor
                  << " | " << result.food.pickup << "\n";
    }
}

} // namespace BestMatch
