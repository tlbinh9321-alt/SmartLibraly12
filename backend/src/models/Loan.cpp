#include "models/Loan.h"

#include "utils/Exceptions.h"

Loan::Loan(int loanId, int memberId, int resourceId, std::string borrowDate, std::string dueDate)
    : loanId_(loanId), memberId_(memberId), resourceId_(resourceId),
      borrowDate_(std::move(borrowDate)), dueDate_(std::move(dueDate)) {}

Loan::Loan(int loanId, int memberId, int resourceId, std::string borrowDate, std::string dueDate,
           std::string returnDate, LoanStatus status, double fineAmount, bool finePaid)
    : loanId_(loanId), memberId_(memberId), resourceId_(resourceId),
      borrowDate_(std::move(borrowDate)), dueDate_(std::move(dueDate)),
      returnDate_(std::move(returnDate)), status_(status), fineAmount_(fineAmount), finePaid_(finePaid) {}

void Loan::markOverdue() {
    if (status_ == LoanStatus::BORROWED) status_ = LoanStatus::OVERDUE;
}

void Loan::markReturned(const std::string& returnDate, double fineAmount) {
    if (status_ == LoanStatus::RETURNED) throw ConflictException("Loan has already been returned.");
    returnDate_ = returnDate;
    fineAmount_ = fineAmount;
    finePaid_ = fineAmount <= 0.0;  // nothing to pay
    status_ = LoanStatus::RETURNED;
}
