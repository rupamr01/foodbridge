#pragma once

#include <string>

// Small date helpers used by the demo data and the best-match score.
// Dates are always stored as YYYY-MM-DD strings, so normal string comparison
// also gives date order. These functions add real calendar arithmetic.
namespace DateUtils {

// True when text is a valid YYYY-MM-DD date.
bool isIsoDate(const std::string& text);

// Today's local date as YYYY-MM-DD.
std::string todayIso();

// Add (or subtract) days from an ISO date. Returns "" for an invalid date.
std::string addDays(const std::string& isoDate, int days);

// Number of days from fromIso to toIso. Positive means toIso is later.
// Returns 0 for invalid input.
int daysBetween(const std::string& fromIso, const std::string& toIso);

} // namespace DateUtils
