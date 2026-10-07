#pragma once
#include <string>

// Dates are ISO strings ("YYYY-MM-DD"): sortable, human readable, easy to persist.
namespace DateUtils {
std::string today();
std::string nowTimestamp();                       // "YYYY-MM-DD HH:MM:SS"
bool isValid(const std::string& isoDate);
int toDayNumber(const std::string& isoDate);      // days since 1970-01-01
std::string fromDayNumber(int dayNumber);
int daysBetween(const std::string& from, const std::string& to);  // to - from
std::string addDays(const std::string& isoDate, int days);
std::string toDayMonthYear(const std::string& isoDate);           // DD/MM/YYYY
}  // namespace DateUtils
