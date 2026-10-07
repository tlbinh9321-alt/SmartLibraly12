#include "utils/StringUtils.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>

namespace StringUtils {

std::string trim(const std::string& text) {
    auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
    auto first = std::find_if_not(text.begin(), text.end(), isSpace);
    auto last = std::find_if_not(text.rbegin(), text.rend(), isSpace).base();
    return first < last ? std::string(first, last) : std::string();
}

std::string toLower(const std::string& text) {
    std::string out = text;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

std::string toUpper(const std::string& text) {
    std::string out = text;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return out;
}

bool containsIgnoreCase(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return true;
    return toLower(haystack).find(toLower(needle)) != std::string::npos;
}

bool equalsIgnoreCase(const std::string& a, const std::string& b) {
    return toLower(a) == toLower(b);
}

bool isValidEmail(const std::string& email) {
    if (email.empty() || email.find(' ') != std::string::npos) return false;
    const auto at = email.find('@');
    if (at == std::string::npos || at == 0 || email.find('@', at + 1) != std::string::npos) return false;
    const std::string domain = email.substr(at + 1);
    const auto dot = domain.find('.');
    return dot != std::string::npos && dot > 0 && domain.back() != '.';
}

std::string normalizeIdentifier(const std::string& text) {
    std::string out;
    for (unsigned char c : text)
        if (std::isalnum(c)) out += static_cast<char>(std::toupper(c));
    return out;
}

std::vector<std::string> split(const std::string& text, char delimiter) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : text) {
        if (c == delimiter) { parts.push_back(current); current.clear(); }
        else current += c;
    }
    parts.push_back(current);
    return parts;
}

std::string formatMoney(double amount) {
    char buffer[64];
    std::snprintf(buffer, sizeof buffer, "%.2f", amount);
    return buffer;
}

double round2(double value) { return std::round(value * 100.0) / 100.0; }

}  // namespace StringUtils
