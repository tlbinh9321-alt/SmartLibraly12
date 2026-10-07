#pragma once
#include <string>

enum class MemberStatus { ACTIVE, INACTIVE, SUSPENDED };
enum class LoanStatus { BORROWED, RETURNED, OVERDUE };
enum class ReservationStatus { WAITING, READY, COMPLETED, CANCELLED };

std::string toString(MemberStatus status);
std::string toString(LoanStatus status);
std::string toString(ReservationStatus status);

// parse* throw ValidationException on unknown text (case-insensitive).
MemberStatus parseMemberStatus(const std::string& text);
LoanStatus parseLoanStatus(const std::string& text);
ReservationStatus parseReservationStatus(const std::string& text);
