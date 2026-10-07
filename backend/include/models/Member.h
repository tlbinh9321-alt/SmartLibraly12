#pragma once
#include <memory>
#include <string>

#include "models/Enums.h"

class FinePolicy;

// ABSTRACT BASE CLASS for library members. Subscription types differ in
// borrow limit, loan duration and fine rules (all virtual).
class Member {
public:
    Member(int memberId, std::string fullName, std::string email, std::string phone,
           MemberStatus status, std::string registrationDate);
    virtual ~Member() = default;

    virtual int getBorrowLimit() const = 0;
    virtual double getFineRate() const = 0;          // multiplier applied to the resource's late fee
    virtual int getLoanDurationDays() const = 0;
    virtual std::string getMemberType() const = 0;   // "STUDENT" | "FACULTY"
    // Factory Method: each member type builds its own fine strategy.
    virtual std::unique_ptr<FinePolicy> createFinePolicy(double resourceRatePerDay) const = 0;

    int getMemberId() const { return memberId_; }
    const std::string& getFullName() const { return fullName_; }
    const std::string& getEmail() const { return email_; }
    const std::string& getPhone() const { return phone_; }
    MemberStatus getStatus() const { return status_; }
    const std::string& getRegistrationDate() const { return registrationDate_; }

    void setStatus(MemberStatus status) { status_ = status; }
    void ensureCanBorrow() const;  // throws if INACTIVE or SUSPENDED

private:
    int memberId_;
    std::string fullName_;
    std::string email_;
    std::string phone_;
    MemberStatus status_;
    std::string registrationDate_;
};

class StudentMember : public Member {
public:
    static constexpr int BORROW_LIMIT = 3;
    static constexpr int LOAN_DAYS = 14;

    using Member::Member;
    int getBorrowLimit() const override { return BORROW_LIMIT; }
    double getFineRate() const override;
    int getLoanDurationDays() const override { return LOAN_DAYS; }
    std::string getMemberType() const override { return "STUDENT"; }
    std::unique_ptr<FinePolicy> createFinePolicy(double resourceRatePerDay) const override;
};

class FacultyMember : public Member {
public:
    static constexpr int BORROW_LIMIT = 10;
    static constexpr int LOAN_DAYS = 30;

    using Member::Member;
    int getBorrowLimit() const override { return BORROW_LIMIT; }
    double getFineRate() const override;
    int getLoanDurationDays() const override { return LOAN_DAYS; }
    std::string getMemberType() const override { return "FACULTY"; }
    std::unique_ptr<FinePolicy> createFinePolicy(double resourceRatePerDay) const override;
};
