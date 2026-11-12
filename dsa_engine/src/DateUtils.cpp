#include "DateUtils.h"

#include <cctype>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

namespace {

bool isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int daysInMonth(int year, int month) {
    static const int days[] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    if (month == 2 && isLeapYear(year)) {
        return 29;
    }
    return days[month - 1];
}

bool parseIso(const std::string& text, int& year, int& month, int& day) {
    if (text.size() != 10 || text[4] != '-' || text[7] != '-') {
        return false;
    }

    for (std::size_t i = 0; i < text.size(); ++i) {
        if (i == 4 || i == 7) {
            continue;
        }
        if (!std::isdigit(static_cast<unsigned char>(text[i]))) {
            return false;
        }
    }

    year = std::stoi(text.substr(0, 4));
    month = std::stoi(text.substr(5, 2));
    day = std::stoi(text.substr(8, 2));

    if (year < 1 || month < 1 || month > 12) {
        return false;
    }
    return day >= 1 && day <= daysInMonth(year, month);
}

// Howard Hinnant's civil-date algorithm.
// Converts a calendar date to a day number so adding days is simple arithmetic.
long long daysFromCivil(int year, int month, int day) {
    int y = year;
    unsigned m = static_cast<unsigned>(month);
    unsigned d = static_cast<unsigned>(day);

    y -= m <= 2;
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;

    return static_cast<long long>(era) * 146097
         + static_cast<long long>(doe) - 719468;
}

void civilFromDays(long long z, int& year, int& month, int& day) {
    z += 719468;

    const long long era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = static_cast<unsigned>(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    const long long y = static_cast<long long>(yoe) + era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    const unsigned d = doy - (153 * mp + 2) / 5 + 1;
    const unsigned m = mp + (mp < 10 ? 3 : -9);

    year = static_cast<int>(y + (m <= 2));
    month = static_cast<int>(m);
    day = static_cast<int>(d);
}

std::string formatIso(int year, int month, int day) {
    std::ostringstream out;
    out << std::setfill('0') << std::setw(4) << year << '-'
        << std::setw(2) << month << '-'
        << std::setw(2) << day;
    return out.str();
}

} // namespace

namespace DateUtils {

bool isIsoDate(const std::string& text) {
    int year = 0;
    int month = 0;
    int day = 0;
    return parseIso(text, year, month, day);
}

std::string todayIso() {
    std::time_t now = std::time(nullptr);
    std::tm* local = std::localtime(&now);
    if (local == nullptr) {
        return "1970-01-01";
    }
    return formatIso(
        local->tm_year + 1900,
        local->tm_mon + 1,
        local->tm_mday
    );
}

std::string addDays(const std::string& isoDate, int days) {
    int year = 0;
    int month = 0;
    int day = 0;
    if (!parseIso(isoDate, year, month, day)) {
        return "";
    }

    long long totalDays = daysFromCivil(year, month, day) + days;
    civilFromDays(totalDays, year, month, day);
    return formatIso(year, month, day);
}

int daysBetween(const std::string& fromIso, const std::string& toIso) {
    int fromYear = 0;
    int fromMonth = 0;
    int fromDay = 0;
    int toYear = 0;
    int toMonth = 0;
    int toDay = 0;

    if (!parseIso(fromIso, fromYear, fromMonth, fromDay)
            || !parseIso(toIso, toYear, toMonth, toDay)) {
        return 0;
    }

    long long fromDays = daysFromCivil(fromYear, fromMonth, fromDay);
    long long toDays = daysFromCivil(toYear, toMonth, toDay);
    return static_cast<int>(toDays - fromDays);
}

} // namespace DateUtils
