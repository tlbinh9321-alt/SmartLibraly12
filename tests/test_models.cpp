// 1 Resource creation, 2 Member creation, 10 Polymorphism, 6 Fine calculation
#include <memory>
#include <vector>

#include "models/Book.h"
#include "models/EBook.h"
#include "models/FinePolicy.h"
#include "models/Journal.h"
#include "test_framework.h"
#include "test_helpers.h"

TEST(resource_creation_all_three_types) {
    TestApp t("res_create");
    auto& book = t->resources.create("BOOK", bookFields("Clean Code", "978-0-13-235088-4", 2));
    auto& ebook = t->resources.create("EBOOK", {{"title", "A Tour of C++"}, {"author", "B. S."}, {"isbn", "978-0-13-499783-4"}, {"genre", "Programming"},
                                                {"publicationYear", "2022"}, {"totalCopies", "5"}, {"fileFormat", "epub"}, {"fileSizeMB", "4.2"}});
    auto& journal = t->resources.create("JOURNAL", {{"title", "Nature"}, {"author", "SN"}, {"genre", "Science"}, {"publicationYear", "2024"},
                                                    {"totalCopies", "1"}, {"issn", "0028-0836"}, {"volume", "630"}, {"issue", "8015"}});
    CHECK_EQ(book.getType(), std::string("BOOK"));
    CHECK_EQ(ebook.getType(), std::string("EBOOK"));
    CHECK_EQ(journal.getType(), std::string("JOURNAL"));
    CHECK_EQ(book.getId(), 1);
    CHECK_EQ(journal.getId(), 3);
    CHECK_EQ(book.getAvailableCopies(), 2);
    CHECK_EQ(journal.getIdentifier(), std::string("0028-0836"));
    CHECK_EQ(journal.getIdentifierLabel(), std::string("ISSN"));
}

TEST(resource_validation_errors) {
    TestApp t("res_invalid");
    CHECK_THROWS(t->resources.create("BOOK", bookFields("A", "978-0-13-235088-4", -1)), ValidationException, "negative");
    CHECK_THROWS(t->resources.create("BOOK", bookFields("A", "12345", 1)), ValidationException, "Invalid ISBN");
    CHECK_THROWS(t->resources.create("BOOK", bookFields("", "978-0-13-235088-4", 1)), ValidationException, "Title is required");
    CHECK_THROWS(t->resources.create("COMIC", bookFields("A", "978-0-13-235088-4", 1)), ValidationException, "Unknown resource type");
    CHECK_THROWS(t->resources.create("JOURNAL", {{"title", "J"}, {"author", "A"}, {"genre", "G"}, {"publicationYear", "2020"}, {"issn", "bad"}}),
                 ValidationException, "Invalid ISSN");
    t->resources.create("BOOK", bookFields("Clean Code", "978-0-13-235088-4"));
    CHECK_THROWS(t->resources.create("BOOK", bookFields("Other", "9780132350884")), ConflictException, "Duplicate ISBN");
    CHECK_EQ(t->library.resources().size(), size_t(1));   // failed creations leave no trace
    CHECK_EQ(t->library.ids().resource, 2);               // ...and burn no ids
}

TEST(resource_update_and_delete_rules) {
    TestApp t("res_update");
    auto& r = t->resources.create("BOOK", bookFields("Old", "978-0-13-235088-4", 2));
    t->members.create("STUDENT", studentFields("S", "s@x.edu"));
    t->loans.borrowResource(1, r.getId());
    CHECK_THROWS(t->resources.update(1, {{"totalCopies", "0"}}), ValidationException, "currently on loan");
    auto& updated = t->resources.update(1, {{"title", "New"}, {"totalCopies", "4"}});
    CHECK_EQ(updated.getTitle(), std::string("New"));
    CHECK_EQ(updated.getTotalCopies(), 4);
    CHECK_EQ(updated.getAvailableCopies(), 3);  // one copy is still out
    CHECK_THROWS(t->resources.update(1, {{"type", "EBOOK"}}), ValidationException, "cannot be changed");
    CHECK_THROWS(t->resources.remove(1), ConflictException, "on loan");
    t->loans.returnResource(1);
    t->resources.remove(1);
    CHECK(t->library.resources().empty());
    CHECK_THROWS(t->resources.getById(1), NotFoundException, "Resource not found");
}

TEST(member_creation_and_validation) {
    TestApp t("member_create");
    auto& s = t->members.create("STUDENT", studentFields("Nguyen Van A", "a@student.edu"));
    auto& f = t->members.create("faculty", studentFields("Dr. B", "b@faculty.edu"));
    CHECK_EQ(s.getMemberType(), std::string("STUDENT"));
    CHECK_EQ(f.getMemberType(), std::string("FACULTY"));
    CHECK(s.getStatus() == MemberStatus::ACTIVE);
    CHECK_THROWS(t->members.create("STUDENT", studentFields("X", "not-an-email")), ValidationException, "Invalid email");
    CHECK_THROWS(t->members.create("STUDENT", studentFields("X", "A@student.edu")), ConflictException, "already exists");
    CHECK_THROWS(t->members.create("ALIEN", studentFields("X", "x@x.edu")), ValidationException, "Unknown member type");
    CHECK_THROWS(t->members.create("STUDENT", studentFields("", "x@x.edu")), ValidationException, "Full name");
    auto fields = studentFields("Y", "y@x.edu");
    fields["status"] = "BANNED";
    CHECK_THROWS(t->members.create("STUDENT", fields), ValidationException, "Invalid member status");
}

TEST(polymorphism_resources_through_base_pointers) {
    std::vector<std::unique_ptr<LibraryResource>> shelf;
    shelf.push_back(std::make_unique<Book>(1, "B", "A", "978-0-13-235088-4", "G", 2020, 1, "P", 100, "1st"));
    shelf.push_back(std::make_unique<EBook>(2, "E", "A", "978-0-13-499783-4", "G", 2020, 1, "pdf", 1.5, "url", "Perpetual"));
    shelf.push_back(std::make_unique<Journal>(3, "J", "A", "G", 2020, 1, 1, 1, "0028-0836", "Science"));
    CHECK_EQ(shelf[0]->getType(), std::string("BOOK"));
    CHECK_EQ(shelf[1]->getType(), std::string("EBOOK"));
    CHECK_EQ(shelf[2]->getType(), std::string("JOURNAL"));
    CHECK_NEAR(shelf[0]->getLateFeeRate(), 0.50);
    CHECK_NEAR(shelf[1]->getLateFeeRate(), 0.20);
    CHECK_NEAR(shelf[2]->getLateFeeRate(), 1.00);
    CHECK(shelf[0]->getDescription().find("Printed book") != std::string::npos);
    CHECK(shelf[1]->getDescription().find("PDF") != std::string::npos);
    CHECK(shelf[2]->getDescription().find("Academic journal") != std::string::npos);
}

TEST(polymorphism_members_have_different_rules) {
    StudentMember s(1, "S", "s@x.edu", "", MemberStatus::ACTIVE, "2026-01-01");
    FacultyMember f(2, "F", "f@x.edu", "", MemberStatus::ACTIVE, "2026-01-01");
    const Member* members[] = {&s, &f};
    CHECK_EQ(members[0]->getBorrowLimit(), 3);
    CHECK_EQ(members[1]->getBorrowLimit(), 10);
    CHECK(members[1]->getBorrowLimit() > members[0]->getBorrowLimit());
    CHECK(members[1]->getLoanDurationDays() > members[0]->getLoanDurationDays());
    CHECK(members[1]->getFineRate() < members[0]->getFineRate());
    CHECK(members[0]->createFinePolicy(0.5)->describe().find("Student") != std::string::npos);
    CHECK(members[1]->createFinePolicy(0.5)->describe().find("Faculty") != std::string::npos);
}

TEST(resource_copy_encapsulation) {
    Book b(1, "B", "A", "978-0-13-235088-4", "G", 2020, 1, "P", 100, "1st");
    b.borrowCopy();
    CHECK(!b.isAvailable());
    CHECK_THROWS(b.borrowCopy(), ValidationException, "unavailable");
    b.returnCopy();
    CHECK_THROWS(b.returnCopy(), ValidationException, "already in the library");
    CHECK_THROWS(b.restoreAvailableCopies(5), ValidationException, "between 0");
}

TEST(fine_policies_calculate_per_member_type_and_resource) {
    TestApp t("fine_policy");
    auto& book = t->resources.create("BOOK", bookFields("B", "978-0-13-235088-4"));
    t->resources.create("EBOOK", {{"title", "E"}, {"author", "A"}, {"isbn", "978-0-13-499783-4"}, {"genre", "G"}, {"publicationYear", "2020"}});
    auto& journal = t->resources.create("JOURNAL", {{"title", "J"}, {"author", "A"}, {"genre", "G"}, {"publicationYear", "2020"}, {"issn", "0028-0836"}});
    auto& student = t->members.create("STUDENT", studentFields("S", "s@x.edu"));
    auto& faculty = t->members.create("FACULTY", studentFields("F", "f@x.edu"));
    CHECK_NEAR(t->fines.calculateFine(student, book, 10), 5.00);      // 10 x 0.50 x 1.0
    CHECK_NEAR(t->fines.calculateFine(student, journal, 10), 10.00);  // journal rate is higher
    CHECK_NEAR(t->fines.calculateFine(faculty, book, 10), 1.75);      // (10-3 grace) x 0.50 x 0.5
    CHECK_NEAR(t->fines.calculateFine(faculty, book, 3), 0.0);        // inside grace period
    CHECK_NEAR(t->fines.calculateFine(student, book, 0), 0.0);
    CHECK_NEAR(t->fines.calculateFine(student, book, -4), 0.0);
}
