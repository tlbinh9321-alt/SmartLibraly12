#pragma once
#include <string>

#include "models/Enums.h"

class Loan {
public:
    Loan(int loanId, int memberId, int resourceId, std::string borrowDate, std::string dueDate);
    // full constructor used when loading saved state
    Loan(int loanId, int memberId, int resourceId, std::string borrowDate, std::string dueDate,
         std::string returnDate, LoanStatus status, double fineAmount, bool finePaid);

    int getLoanId() const { return loanId_; }
    int getMemberId() const { return memberId_; }
    int getResourceId() const { return resourceId_; }
    const std::string& getBorrowDate() const { return borrowDate_; }
    const std::string& getDueDate() const { return dueDate_; }
    const std::string& getReturnDate() const { return returnDate_; }  // empty until returned
    LoanStatus getStatus() const { return status_; }
    double getFineAmount() const { return fineAmount_; }
    bool isFinePaid() const { return finePaid_; }

    bool isActive() const { return status_ != LoanStatus::RETURNED; }
    bool isReturned() const { return status_ == LoanStatus::RETURNED; }

    void markOverdue();
    void markReturned(const std::string& returnDate, double fineAmount);
    void markFinePaid() { finePaid_ = true; }

private:
    int loanId_;
    int memberId_;
    int resourceId_;
    std::string borrowDate_;
    std::string dueDate_;
    std::string returnDate_;
    LoanStatus status_ = LoanStatus::BORROWED;
    double fineAmount_ = 0.0;
    bool finePaid_ = false;
};
