#pragma once
#include <string>

// Strategy interface for late fines. Two independent polymorphic axes combine:
//   * the RESOURCE decides the base rate per day  (LibraryResource::getLateFeeRate)
//   * the MEMBER   decides how that rate is applied (Member::createFinePolicy)
// This keeps "fine = overdueDays x rate" out of controllers and avoids a
// StudentBookPolicy / FacultyJournalPolicy class explosion.
class FinePolicy {
public:
    virtual ~FinePolicy() = default;
    virtual double calculateFine(int overdueDays) const = 0;
    virtual std::string describe() const = 0;
};

// Students pay the full resource rate from the first overdue day.
class StudentFinePolicy : public FinePolicy {
public:
    static constexpr double RATE_FACTOR = 1.0;
    explicit StudentFinePolicy(double resourceRatePerDay);
    double calculateFine(int overdueDays) const override;
    std::string describe() const override;

private:
    double ratePerDay_;
};

// Faculty get a grace period and a discounted rate.
class FacultyFinePolicy : public FinePolicy {
public:
    static constexpr double RATE_FACTOR = 0.5;
    static constexpr int GRACE_DAYS = 3;
    explicit FacultyFinePolicy(double resourceRatePerDay);
    double calculateFine(int overdueDays) const override;
    std::string describe() const override;

private:
    double ratePerDay_;
};
