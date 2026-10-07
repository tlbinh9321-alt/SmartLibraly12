// 3 Borrowing, 4 Returning, 5 Borrow limit, 6 Fine on return, 8 Reservation queue
#include "services/LibraryApp.h"
#include "test_framework.h"
#include "test_helpers.h"
#include "utils/DateUtils.h"

namespace {
struct Fixture {
    TestApp t;
    int student = 0, student2 = 0, student3 = 0, faculty = 0;
    explicit Fixture(const std::string& name) : t(name) {
        student = t->members.create("STUDENT", studentFields("Student One", "s1@x.edu")).getMemberId();
        student2 = t->members.create("STUDENT", studentFields("Student Two", "s2@x.edu")).getMemberId();
        student3 = t->members.create("STUDENT", studentFields("Student Three", "s3@x.edu")).getMemberId();
        faculty = t->members.create("FACULTY", studentFields("Faculty One", "f1@x.edu")).getMemberId();
    }
    int addBook(const std::string& title, const std::string& isbn, int copies = 1) {
        return t->resources.create("BOOK", bookFields(title, isbn, copies)).getId();
    }
};
}  // namespace

TEST(borrow_decreases_copies_and_sets_due_date) {
    Fixture f("borrow_ok");
    const int book = f.addBook("B1", "978-0-13-235088-4", 2);
    Loan loan = f.t->loans.borrowResource(f.student, book, "2026-03-01");
    CHECK_EQ(f.t->library.findResource(book)->getAvailableCopies(), 1);
    CHECK_EQ(loan.getDueDate(), std::string("2026-03-15"));  // students: 14 days
    CHECK(loan.getStatus() == LoanStatus::BORROWED);
    Loan facultyLoan = f.t->loans.borrowResource(f.faculty, book, "2026-03-01");
    CHECK_EQ(facultyLoan.getDueDate(), std::string("2026-03-31"));  // faculty: 30 days
}

TEST(borrow_validation_messages) {
    Fixture f("borrow_invalid");
    const int book = f.addBook("B1", "978-0-13-235088-4");
    CHECK_THROWS(f.t->loans.borrowResource(999, book), NotFoundException, "Member not found.");
    CHECK_THROWS(f.t->loans.borrowResource(f.student, 999), NotFoundException, "Resource not found.");
    f.t->members.update(f.student2, {{"status", "INACTIVE"}});
    f.t->members.update(f.student3, {{"status", "SUSPENDED"}});
    CHECK_THROWS(f.t->loans.borrowResource(f.student2, book), ValidationException, "Member account is inactive.");
    CHECK_THROWS(f.t->loans.borrowResource(f.student3, book), ValidationException, "suspended");
    f.t->loans.borrowResource(f.student, book);
    CHECK_THROWS(f.t->loans.borrowResource(f.faculty, book), ValidationException, "Resource is currently unavailable.");
    CHECK_THROWS(f.t->loans.borrowResource(f.student, book), ConflictException, "already has this resource");
}

TEST(student_cannot_borrow_when_limit_reached_but_faculty_can) {
    Fixture f("limit");
    std::vector<int> books;
    for (int i = 0; i < 5; ++i) books.push_back(f.addBook("Book " + std::to_string(i), "978-0-13-23508" + std::to_string(i) + "-" + std::to_string(i), 3));
    for (int i = 0; i < 3; ++i) f.t->loans.borrowResource(f.student, books[i]);
    CHECK_THROWS(f.t->loans.borrowResource(f.student, books[3]), ValidationException, "Borrowing limit reached.");
    for (int i = 0; i < 5; ++i) f.t->loans.borrowResource(f.faculty, books[i]);  // faculty limit is 10
    CHECK_EQ(f.t->loans.countActiveLoans(f.faculty), 5);
    // returning frees a slot again
    f.t->loans.returnResource(1);
    f.t->loans.borrowResource(f.student, books[3]);
}

TEST(return_restores_copy_and_rejects_invalid_returns) {
    Fixture f("return");
    const int book = f.addBook("B1", "978-0-13-235088-4");
    Loan loan = f.t->loans.borrowResource(f.student, book);
    CHECK_THROWS(f.t->loans.returnResource(loan.getLoanId(), f.student2), ValidationException, "does not belong");
    CHECK_THROWS(f.t->loans.returnResource(999), NotFoundException, "Loan not found.");
    ReturnResult result = f.t->loans.returnResource(loan.getLoanId(), f.student);
    CHECK(result.loan.getStatus() == LoanStatus::RETURNED);
    CHECK_EQ(result.overdueDays, 0);
    CHECK_NEAR(result.fine, 0.0);
    CHECK_EQ(f.t->library.findResource(book)->getAvailableCopies(), 1);
    CHECK_THROWS(f.t->loans.returnResource(loan.getLoanId()), ConflictException, "already been returned");
    CHECK_THROWS(f.t->loans.returnResource(loan.getLoanId(), std::nullopt, "2000-01-01"), ConflictException, "already");
}

TEST(returning_overdue_resource_calculates_fine) {
    Fixture f("overdue_fine");
    const int book = f.addBook("B1", "978-0-13-235088-4", 2);
    Loan studentLoan = f.t->loans.borrowResource(f.student, book, "2026-01-01");  // due 2026-01-15
    Loan facultyLoan = f.t->loans.borrowResource(f.faculty, book, "2026-01-01");   // due 2026-01-31
    ReturnResult s = f.t->loans.returnResource(studentLoan.getLoanId(), std::nullopt, "2026-01-25");
    CHECK_EQ(s.overdueDays, 10);
    CHECK_NEAR(s.fine, 5.00);
    ReturnResult fac = f.t->loans.returnResource(facultyLoan.getLoanId(), std::nullopt, "2026-02-10");
    CHECK_EQ(fac.overdueDays, 10);
    CHECK_NEAR(fac.fine, 1.75);
    CHECK_NEAR(f.t->loans.totalOutstandingFines(), 6.75);
    f.t->loans.payFine(studentLoan.getLoanId());
    CHECK_NEAR(f.t->loans.totalOutstandingFines(), 1.75);
    CHECK_NEAR(f.t->loans.totalCollectedFines(), 5.00);
    CHECK_THROWS(f.t->loans.payFine(studentLoan.getLoanId()), ConflictException, "already been paid");
    CHECK_THROWS(f.t->loans.returnResource(f.t->loans.borrowResource(f.student, book).getLoanId(), std::nullopt, "2000-01-01"),
                 ValidationException, "before the borrow date");
}

TEST(overdue_status_and_preview) {
    Fixture f("overdue_status");
    const int book = f.addBook("B1", "978-0-13-235088-4");
    const std::string today = DateUtils::today();
    Loan loan = f.t->loans.borrowResource(f.student, book, DateUtils::addDays(today, -20));
    CHECK_EQ(f.t->loans.getOverdueLoans().size(), size_t(1));
    CHECK_EQ(f.t->loans.getActiveLoans().size(), size_t(1));
    ReturnPreview preview = f.t->loans.previewReturn(loan.getLoanId());
    CHECK_EQ(preview.overdueDays, 6);
    CHECK_NEAR(preview.fine, 3.00);
    CHECK_EQ(f.t->loans.calculateOverdueDays(loan, DateUtils::addDays(today, -10)), 0);  // not yet due then
}

TEST(unavailable_resource_can_be_reserved_and_rules_apply) {
    Fixture f("reserve_rules");
    const int book = f.addBook("B1", "978-0-13-235088-4");
    CHECK_THROWS(f.t->reservations.reserve(f.student2, book), ValidationException, "borrow it instead");
    f.t->loans.borrowResource(f.student, book);
    Reservation r = f.t->reservations.reserve(f.student2, book);
    CHECK(r.getStatus() == ReservationStatus::WAITING);
    CHECK_THROWS(f.t->reservations.reserve(f.student2, book), ConflictException, "Duplicate reservation");
    CHECK_THROWS(f.t->reservations.reserve(f.student, book), ConflictException, "already has this resource on loan");
    CHECK_THROWS(f.t->reservations.reserve(999, book), NotFoundException, "Member not found");
    CHECK_THROWS(f.t->reservations.reserve(f.student2, 999), NotFoundException, "Resource not found");
    CHECK_EQ(f.t->reservations.getQueue(book).size(), size_t(1));
}

TEST(first_reservation_receives_priority_and_copy_is_held) {
    Fixture f("reserve_priority");
    const int book = f.addBook("B1", "978-0-13-235088-4");
    Loan loan = f.t->loans.borrowResource(f.student, book);
    Reservation first = f.t->reservations.reserve(f.student2, book);
    Reservation second = f.t->reservations.reserve(f.student3, book);
    CHECK_EQ(f.t->reservations.getQueue(book).front().getReservationId(), first.getReservationId());

    ReturnResult result = f.t->loans.returnResource(loan.getLoanId());
    CHECK_EQ(result.promoted.size(), size_t(1));
    CHECK_EQ(result.promoted[0].getMemberId(), f.student2);       // first in line
    CHECK(result.promoted[0].getStatus() == ReservationStatus::READY);
    CHECK_EQ(f.t->reservations.getQueue(book).size(), size_t(1));  // second still waiting

    // The returned copy is held: nobody else may borrow it, not even a faculty member.
    CHECK_EQ(f.t->library.findResource(book)->getAvailableCopies(), 1);
    CHECK_THROWS(f.t->loans.borrowResource(f.faculty, book), ValidationException, "reserved for members");
    CHECK_THROWS(f.t->loans.borrowResource(f.student3, book), ValidationException, "reserved for members");

    // The READY member can borrow, which completes the reservation.
    f.t->loans.borrowResource(f.student2, book);
    bool completed = false;
    for (const Reservation& r : f.t->reservations.getAll())
        if (r.getReservationId() == first.getReservationId()) completed = r.getStatus() == ReservationStatus::COMPLETED;
    CHECK(completed);
    CHECK_EQ(second.getMemberId(), f.student3);
}

TEST(cancelling_reservations_releases_held_copy_in_order) {
    Fixture f("reserve_cancel");
    const int book = f.addBook("B1", "978-0-13-235088-4");
    Loan loan = f.t->loans.borrowResource(f.student, book);
    Reservation a = f.t->reservations.reserve(f.student2, book);
    Reservation b = f.t->reservations.reserve(f.student3, book);
    f.t->loans.returnResource(loan.getLoanId());                 // a is READY
    f.t->reservations.cancel(a.getReservationId());              // copy goes to b
    CHECK(f.t->reservations.findReady(f.student3, book) != nullptr);
    CHECK_THROWS(f.t->reservations.cancel(a.getReservationId()), ConflictException, "already completed or cancelled");
    CHECK_THROWS(f.t->reservations.cancel(999), NotFoundException, "Reservation not found");
    CHECK_EQ(b.getMemberId(), f.student3);
    // cancelling from the middle of a waiting queue keeps FIFO order of the rest
    const int other = f.addBook("B2", "978-0-13-499783-4");
    f.t->loans.borrowResource(f.faculty, other);
    Reservation r1 = f.t->reservations.reserve(f.student, other);
    Reservation r2 = f.t->reservations.reserve(f.student2, other);
    Reservation r3 = f.t->reservations.reserve(f.student3, other);
    f.t->reservations.cancel(r2.getReservationId());
    auto queue = f.t->reservations.getQueue(other);
    CHECK_EQ(queue.size(), size_t(2));
    CHECK_EQ(queue[0].getReservationId(), r1.getReservationId());
    CHECK_EQ(queue[1].getReservationId(), r3.getReservationId());
}
