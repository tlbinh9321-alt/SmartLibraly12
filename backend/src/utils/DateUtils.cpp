#include "utils/DateUtils.h"

#include <chrono>
#include <cstdio>
#include <ctime>

#include "utils/Exceptions.h"

namespace {
// Howard Hinnant's public-domain civil calendar algorithms.
long daysFromCivil(int y, unsigned m, unsigned d) {
    y -= m <= 2;
    const long era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<long>(doe) - 719468;
}

void civilFromDays(long z, int& y, unsigned& m, unsigned& d) {
    z += 719468;
    const long era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = static_cast<unsigned>(z - era * 146097);
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    y = static_cast<int>(yoe) + static_cast<int>(era) * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    const unsigned mp = (5 * doy + 2) / 153;
    d = doy - (153 * mp + 2) / 5 + 1;
    m = mp < 10 ? mp + 3 : mp - 9;
    y += (m <= 2);
}

std::tm localTime() {
    const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm result{};
#ifdef _WIN32
    localtime_s(&result, &now);
#else
    localtime_r(&now, &result);
#endif
    return result;
}
}  // namespace

namespace DateUtils {

std::string today() {
    const std::tm t = localTime();
    char buffer[48];
    std::snprintf(buffer, sizeof buffer, "%04d-%02d-%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
    return buffer;
}

std::string nowTimestamp() {
    const std::tm t = localTime();
    char buffer[96];
    std::snprintf(buffer, sizeof buffer, "%04d-%02d-%02d %02d:%02d:%02d", t.tm_year + 1900,
                  t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
    return buffer;
}

bool isValid(const std::string& isoDate) {
    int y = 0, m = 0, d = 0;
    if (isoDate.size() != 10 || std::sscanf(isoDate.c_str(), "%4d-%2d-%2d", &y, &m, &d) != 3) return false;
    if (m < 1 || m > 12 || d < 1 || d > 31 || y < 1900 || y > 2200) return false;
    return fromDayNumber(toDayNumber(isoDate)) == isoDate;  // rejects 2026-02-31
}

int toDayNumber(const std::string& isoDate) {
    int y = 0, m = 0, d = 0;
    if (std::sscanf(isoDate.c_str(), "%d-%d-%d", &y, &m, &d) != 3)
        throw ValidationException("Invalid date: '" + isoDate + "' (expected YYYY-MM-DD).");
    return static_cast<int>(daysFromCivil(y, static_cast<unsigned>(m), static_cast<unsigned>(d)));
}

std::string fromDayNumber(int dayNumber) {
    int y = 0;
    unsigned m = 0, d = 0;
    civilFromDays(dayNumber, y, m, d);
    char buffer[48];
    std::snprintf(buffer, sizeof buffer, "%04d-%02u-%02u", y, m, d);
    return buffer;
}

int daysBetween(const std::string& from, const std::string& to) { return toDayNumber(to) - toDayNumber(from); }

std::string addDays(const std::string& isoDate, int days) { return fromDayNumber(toDayNumber(isoDate) + days); }

std::string toDayMonthYear(const std::string& isoDate) {
    if (isoDate.size() != 10) return isoDate;
    return isoDate.substr(8, 2) + "/" + isoDate.substr(5, 2) + "/" + isoDate.substr(0, 4);
}

}  // namespace DateUtils
