#include "models/Enums.h"

#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

std::string toString(MemberStatus s) {
    switch (s) {
        case MemberStatus::ACTIVE: return "ACTIVE";
        case MemberStatus::INACTIVE: return "INACTIVE";
        case MemberStatus::SUSPENDED: return "SUSPENDED";
    }
    return "ACTIVE";
}
std::string toString(LoanStatus s) {
    switch (s) {
        case LoanStatus::BORROWED: return "BORROWED";
        case LoanStatus::RETURNED: return "RETURNED";
        case LoanStatus::OVERDUE: return "OVERDUE";
    }
    return "BORROWED";
}
std::string toString(ReservationStatus s) {
    switch (s) {
        case ReservationStatus::WAITING: return "WAITING";
        case ReservationStatus::READY: return "READY";
        case ReservationStatus::COMPLETED: return "COMPLETED";
        case ReservationStatus::CANCELLED: return "CANCELLED";
    }
    return "WAITING";
}

MemberStatus parseMemberStatus(const std::string& text) {
    const std::string t = StringUtils::toUpper(StringUtils::trim(text));
    if (t == "ACTIVE") return MemberStatus::ACTIVE;
    if (t == "INACTIVE") return MemberStatus::INACTIVE;
    if (t == "SUSPENDED") return MemberStatus::SUSPENDED;
    throw ValidationException("Invalid member status '" + text + "' (expected ACTIVE, INACTIVE or SUSPENDED).");
}
LoanStatus parseLoanStatus(const std::string& text) {
    const std::string t = StringUtils::toUpper(StringUtils::trim(text));
    if (t == "BORROWED") return LoanStatus::BORROWED;
    if (t == "RETURNED") return LoanStatus::RETURNED;
    if (t == "OVERDUE") return LoanStatus::OVERDUE;
    throw ValidationException("Invalid loan status '" + text + "'.");
}
ReservationStatus parseReservationStatus(const std::string& text) {
    const std::string t = StringUtils::toUpper(StringUtils::trim(text));
    if (t == "WAITING") return ReservationStatus::WAITING;
    if (t == "READY") return ReservationStatus::READY;
    if (t == "COMPLETED") return ReservationStatus::COMPLETED;
    if (t == "CANCELLED") return ReservationStatus::CANCELLED;
    throw ValidationException("Invalid reservation status '" + text + "'.");
}
