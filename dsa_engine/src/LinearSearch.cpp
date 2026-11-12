#include "Searching.h"

#include <cctype>
#include <string>
#include <vector>

// This file contains one searching algorithm: linear search.
namespace Searching {

std::string toLower(std::string text) {
    for (std::size_t i = 0; i < text.size(); ++i) {
        text[i] = static_cast<char>(
            std::tolower(static_cast<unsigned char>(text[i]))
        );
    }
    return text;
}

std::vector<Food> linearSearchByName(
    const std::vector<Food>& foods,
    const std::string& query
) {
    std::vector<Food> matches;
    std::string needle = toLower(query);

    for (const Food& food : foods) {
        if (toLower(food.name).find(needle) != std::string::npos) {
            matches.push_back(food);
        }
    }
    return matches;
}

} // namespace Searching
