#pragma once
#include <optional>
#include <string>
#include <vector>

#include "models/Library.h"
#include "services/FineService.h"
#include "services/ReservationService.h"

struct ReturnPreview {
    int loanId;
    std::string dueDate;
    std::string asOfDate;
    int overdueDays;
    double fine;
};

struct ReturnResult {
    Loan loan;
    int overdueDays;
    double fine;
    std::vector<Reservation> promoted;  // reservations that just became READY
};

// Borrow / return workflow. Dates default to "today"; tests and demo data pass
// explicit dates so overdue behaviour is deterministic.
class LoanService {
public:
    LoanService(Library& library, const FineService& fines, ReservationService& reservations)
        : library_(library), fines_(fines), reservations_(reservations) {}

    Loan borrowResource(int memberId, int resourceId, const std::string& borrowDate = "");
    ReturnResult returnResource(int loanId, std::optional<int> memberId = std::nullopt,
                                const std::string& returnDate = "");
    ReturnPreview previewReturn(int loanId, const std::string& asOfDate = "");
    void payFine(int loanId);

    int calculateOverdueDays(const Loan& loan, const std::string& asOfDate = "") const;
    std::vector<Loan> getAllLoans(const std::string& asOfDate = "");
    std::vector<Loan> getActiveLoans(const std::string& asOfDate = "");
    std::vector<Loan> getOverdueLoans(const std::string& asOfDate = "");
    std::vector<Loan> getLoansForMember(int memberId, const std::string& asOfDate = "");
    int countActiveLoans(int memberId) const;

    double outstandingFine(const Loan& loan, const std::string& asOfDate = "") const;
    double outstandingFinesForMember(int memberId, const std::string& asOfDate = "") const;
    double totalOutstandingFines(const std::string& asOfDate = "") const;
    double totalCollectedFines() const;

private:
    void refreshOverdueStatuses(const std::string& asOfDate);
    double accruedFine(const Loan& loan, const std::string& asOfDate) const;

    Library& library_;
    const FineService& fines_;
    ReservationService& reservations_;
};
