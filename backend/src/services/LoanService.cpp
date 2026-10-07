#include "services/LoanService.h"

#include <algorithm>
#include <iterator>

#include "utils/DateUtils.h"
#include "utils/Exceptions.h"

namespace {
std::string orToday(const std::string& date) { return date.empty() ? DateUtils::today() : date; }
}  // namespace

int LoanService::countActiveLoans(int memberId) const {
    const auto& loans = library_.loans();
    return static_cast<int>(std::count_if(loans.begin(), loans.end(),
                                          [memberId](const Loan& l) { return l.isActive() && l.getMemberId() == memberId; }));
}

Loan LoanService::borrowResource(int memberId, int resourceId, const std::string& borrowDate) {
    // 1-3. member exists and may borrow
    Member* member = library_.findMember(memberId);
    if (!member) throw NotFoundException("Member not found.");
    member->ensureCanBorrow();
    // 4-5. borrowing limit
    if (countActiveLoans(memberId) >= member->getBorrowLimit()) throw ValidationException("Borrowing limit reached.");
    // 6-7. resource exists and a copy is free for this member
    LibraryResource* resource = library_.findResource(resourceId);
    if (!resource) throw NotFoundException("Resource not found.");
    for (const Loan& loan : library_.loans())
        if (loan.isActive() && loan.getMemberId() == memberId && loan.getResourceId() == resourceId)
            throw ConflictException("Member already has this resource on loan.");

    const Reservation* ready = reservations_.findReady(memberId, resourceId);
    const int readyReservationId = ready ? ready->getReservationId() : 0;
    if (!ready) {  // copies held for READY reservations are not up for grabs
        if (!resource->isAvailable()) throw ValidationException("Resource is currently unavailable.");
        if (reservations_.effectiveAvailableCopies(*resource) <= 0)
            throw ValidationException("Remaining copies are reserved for members at the front of the waiting queue.");
    }

    const std::string date = orToday(borrowDate);
    if (!DateUtils::isValid(date)) throw ValidationException("Invalid borrow date (expected YYYY-MM-DD).");
    // 8-9. create loan, decrease availableCopies
    Loan loan(library_.ids().loan, memberId, resourceId, date, DateUtils::addDays(date, member->getLoanDurationDays()));
    resource->borrowCopy();
    ++library_.ids().loan;
    if (readyReservationId != 0) reservations_.complete(readyReservationId);
    library_.loans().push_back(loan);
    // 10. save state
    library_.notifyChanged();
    return loan;
}

int LoanService::calculateOverdueDays(const Loan& loan, const std::string& asOfDate) const {
    const std::string reference = loan.isReturned() ? loan.getReturnDate() : orToday(asOfDate);
    return std::max(0, DateUtils::daysBetween(loan.getDueDate(), reference));
}

ReturnPreview LoanService::previewReturn(int loanId, const std::string& asOfDate) {
    const Loan* loan = library_.findLoan(loanId);
    if (!loan) throw NotFoundException("Loan not found.");
    if (loan->isReturned()) throw ConflictException("Loan has already been returned.");
    const Member* member = library_.findMember(loan->getMemberId());
    const LibraryResource* resource = library_.findResource(loan->getResourceId());
    if (!member || !resource) throw NotFoundException("The member or resource of this loan no longer exists.");
    const std::string date = orToday(asOfDate);
    const int overdue = calculateOverdueDays(*loan, date);
    return {loanId, loan->getDueDate(), date, overdue, fines_.calculateFine(*member, *resource, overdue)};
}

ReturnResult LoanService::returnResource(int loanId, std::optional<int> memberId, const std::string& returnDate) {
    Loan* loan = library_.findLoan(loanId);                                           // 1
    if (!loan) throw NotFoundException("Loan not found.");
    if (memberId && *memberId != loan->getMemberId())                                 // 2
        throw ValidationException("This loan does not belong to the specified member.");
    if (loan->isReturned()) throw ConflictException("Loan has already been returned.");  // 3
    Member* member = library_.findMember(loan->getMemberId());
    LibraryResource* resource = library_.findResource(loan->getResourceId());
    if (!member || !resource) throw NotFoundException("The member or resource of this loan no longer exists.");

    const std::string date = orToday(returnDate);
    if (!DateUtils::isValid(date)) throw ValidationException("Invalid return date (expected YYYY-MM-DD).");
    if (DateUtils::daysBetween(loan->getBorrowDate(), date) < 0)
        throw ValidationException("Return date cannot be before the borrow date.");

    const int overdueDays = calculateOverdueDays(*loan, date);                        // 4
    const double fine = fines_.calculateFine(*member, *resource, overdueDays);        // 5
    resource->returnCopy();
    loan->markReturned(date, fine);                                                   // 6, 7
    std::vector<Reservation> promoted = reservations_.fulfillQueue(resource->getId());  // 8
    library_.notifyChanged();                                                         // 9
    return {*loan, overdueDays, fine, promoted};
}

void LoanService::refreshOverdueStatuses(const std::string& asOfDate) {
    const std::string today = orToday(asOfDate);
    for (Loan& loan : library_.loans())
        if (loan.getStatus() == LoanStatus::BORROWED && DateUtils::daysBetween(loan.getDueDate(), today) > 0)
            loan.markOverdue();
}

std::vector<Loan> LoanService::getAllLoans(const std::string& asOfDate) {
    refreshOverdueStatuses(asOfDate);
    return library_.loans();
}

std::vector<Loan> LoanService::getActiveLoans(const std::string& asOfDate) {
    refreshOverdueStatuses(asOfDate);
    std::vector<Loan> active;
    std::copy_if(library_.loans().begin(), library_.loans().end(), std::back_inserter(active),
                 [](const Loan& l) { return l.isActive(); });
    return active;
}

std::vector<Loan> LoanService::getOverdueLoans(const std::string& asOfDate) {
    refreshOverdueStatuses(asOfDate);
    std::vector<Loan> overdue;
    std::copy_if(library_.loans().begin(), library_.loans().end(), std::back_inserter(overdue),
                 [](const Loan& l) { return l.getStatus() == LoanStatus::OVERDUE; });
    return overdue;
}

std::vector<Loan> LoanService::getLoansForMember(int memberId, const std::string& asOfDate) {
    refreshOverdueStatuses(asOfDate);
    std::vector<Loan> mine;
    std::copy_if(library_.loans().begin(), library_.loans().end(), std::back_inserter(mine),
                 [memberId](const Loan& l) { return l.getMemberId() == memberId; });
    return mine;
}

double LoanService::accruedFine(const Loan& loan, const std::string& asOfDate) const {
    const Member* member = library_.findMember(loan.getMemberId());
    const LibraryResource* resource = library_.findResource(loan.getResourceId());
    if (!member || !resource) return 0.0;
    return fines_.calculateFine(*member, *resource, calculateOverdueDays(loan, asOfDate));
}

double LoanService::outstandingFine(const Loan& loan, const std::string& asOfDate) const {
    if (loan.isReturned()) return loan.isFinePaid() ? 0.0 : loan.getFineAmount();
    return accruedFine(loan, asOfDate);  // still on loan: fine accrues until it is returned
}

double LoanService::outstandingFinesForMember(int memberId, const std::string& asOfDate) const {
    double total = 0.0;
    for (const Loan& loan : library_.loans())
        if (loan.getMemberId() == memberId) total += outstandingFine(loan, asOfDate);
    return total;
}

double LoanService::totalOutstandingFines(const std::string& asOfDate) const {
    double total = 0.0;
    for (const Loan& loan : library_.loans()) total += outstandingFine(loan, asOfDate);
    return total;
}

double LoanService::totalCollectedFines() const {
    double total = 0.0;
    for (const Loan& loan : library_.loans())
        if (loan.isReturned() && loan.isFinePaid()) total += loan.getFineAmount();
    return total;
}

void LoanService::payFine(int loanId) {
    Loan* loan = library_.findLoan(loanId);
    if (!loan) throw NotFoundException("Loan not found.");
    if (!loan->isReturned()) throw ValidationException("Return the resource before paying its fine.");
    if (loan->getFineAmount() <= 0.0) throw ValidationException("There is no fine to pay for this loan.");
    if (loan->isFinePaid()) throw ConflictException("The fine for this loan has already been paid.");
    loan->markFinePaid();
    library_.notifyChanged();
}
