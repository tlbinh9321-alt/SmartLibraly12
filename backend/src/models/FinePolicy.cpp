#include "models/FinePolicy.h"

#include <algorithm>

#include "utils/StringUtils.h"

StudentFinePolicy::StudentFinePolicy(double rate) : ratePerDay_(rate) {}

double StudentFinePolicy::calculateFine(int overdueDays) const {
    return StringUtils::round2(std::max(0, overdueDays) * ratePerDay_ * RATE_FACTOR);
}
std::string StudentFinePolicy::describe() const {
    return "Student: $" + StringUtils::formatMoney(ratePerDay_ * RATE_FACTOR) + " per overdue day";
}

FacultyFinePolicy::FacultyFinePolicy(double rate) : ratePerDay_(rate) {}

double FacultyFinePolicy::calculateFine(int overdueDays) const {
    const int billableDays = std::max(0, overdueDays - GRACE_DAYS);
    return StringUtils::round2(billableDays * ratePerDay_ * RATE_FACTOR);
}
std::string FacultyFinePolicy::describe() const {
    return "Faculty: " + std::to_string(GRACE_DAYS) + " grace days, then $" +
           StringUtils::formatMoney(ratePerDay_ * RATE_FACTOR) + " per overdue day";
}
