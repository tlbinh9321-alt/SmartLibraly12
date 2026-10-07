#include "models/Member.h"

#include "models/FinePolicy.h"
#include "utils/DateUtils.h"
#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

Member::Member(int memberId, std::string fullName, std::string email, std::string phone,
               MemberStatus status, std::string registrationDate)
    : memberId_(memberId),
      fullName_(StringUtils::trim(fullName)),
      email_(StringUtils::trim(email)),
      phone_(StringUtils::trim(phone)),
      status_(status),
      registrationDate_(StringUtils::trim(registrationDate)) {
    if (fullName_.empty()) throw ValidationException("Full name is required.");
    if (!StringUtils::isValidEmail(email_)) throw ValidationException("Invalid email address.");
    if (!DateUtils::isValid(registrationDate_)) throw ValidationException("Invalid registration date (expected YYYY-MM-DD).");
    for (unsigned char c : phone_)
        if (!(std::isdigit(c) || c == '+' || c == '-' || c == ' ' || c == '(' || c == ')'))
            throw ValidationException("Invalid phone number.");
}

void Member::ensureCanBorrow() const {
    if (status_ == MemberStatus::INACTIVE) throw ValidationException("Member account is inactive.");
    if (status_ == MemberStatus::SUSPENDED) throw ValidationException("Member account is suspended.");
}

double StudentMember::getFineRate() const { return StudentFinePolicy::RATE_FACTOR; }
std::unique_ptr<FinePolicy> StudentMember::createFinePolicy(double rate) const {
    return std::make_unique<StudentFinePolicy>(rate);
}

double FacultyMember::getFineRate() const { return FacultyFinePolicy::RATE_FACTOR; }
std::unique_ptr<FinePolicy> FacultyMember::createFinePolicy(double rate) const {
    return std::make_unique<FacultyFinePolicy>(rate);
}
