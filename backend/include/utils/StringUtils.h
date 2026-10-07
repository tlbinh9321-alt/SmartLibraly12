#pragma once
#include <string>
#include <vector>

namespace StringUtils {
std::string trim(const std::string& text);
std::string toLower(const std::string& text);   // ASCII case folding
std::string toUpper(const std::string& text);
bool containsIgnoreCase(const std::string& haystack, const std::string& needle);
bool equalsIgnoreCase(const std::string& a, const std::string& b);
bool isValidEmail(const std::string& email);
std::string normalizeIdentifier(const std::string& text);  // "978-0-13" -> "978013"
std::vector<std::string> split(const std::string& text, char delimiter);
std::string formatMoney(double amount);                    // 2 decimals
double round2(double value);
}  // namespace StringUtils
