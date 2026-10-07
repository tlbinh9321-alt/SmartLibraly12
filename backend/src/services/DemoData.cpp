// DEMO / SAMPLE DATA - invented for demonstrations only. Everything is created
// through the real services, so it obeys exactly the same business rules as
// data typed in by a user. Dates are relative to "today" so the demo always
// contains overdue loans.
#include <string>

#include "services/LibraryApp.h"
#include "utils/DateUtils.h"

void LibraryApp::seedDemoData() {
    library.muteNotifications(true);  // save once at the end instead of after every step
    library.clear();
    library.setName("Smart Library (Demo)");

    const std::string today = DateUtils::today();
    auto ago = [&](int days) { return DateUtils::addDays(today, -days); };

    // ---- resources: 7 books, 3 e-books, 3 journals ----
    auto book = [&](const char* title, const char* author, const char* isbn, const char* genre, int year, int copies,
                    const char* publisher, int pages, const char* edition) {
        resources.create("BOOK", {{"title", title}, {"author", author}, {"isbn", isbn}, {"genre", genre},
                                  {"publicationYear", std::to_string(year)}, {"totalCopies", std::to_string(copies)},
                                  {"publisher", publisher}, {"pageCount", std::to_string(pages)}, {"edition", edition}});
    };
    book("Clean Code", "Robert C. Martin", "978-0-13-235088-4", "Software Engineering", 2008, 2, "Prentice Hall", 464, "1st");
    book("The C++ Programming Language", "Bjarne Stroustrup", "978-0-321-56384-2", "Programming", 2013, 3, "Addison-Wesley", 1376, "4th");
    book("Effective Modern C++", "Scott Meyers", "978-1-4919-0399-5", "Programming", 2014, 2, "O'Reilly", 334, "1st");
    book("Design Patterns", "Erich Gamma, Richard Helm, Ralph Johnson, John Vlissides", "978-0-201-63361-0", "Software Engineering", 1994, 1, "Addison-Wesley", 395, "1st");
    book("Introduction to Algorithms", "Thomas H. Cormen et al.", "978-0-262-03384-8", "Computer Science", 2009, 2, "MIT Press", 1292, "3rd");
    book("Harry Potter and the Philosopher's Stone", "J.K. Rowling", "978-0-7475-3269-9", "Fantasy", 1997, 2, "Bloomsbury", 223, "1st");
    book("The Pragmatic Programmer", "Andrew Hunt, David Thomas", "978-0-13-595705-9", "Software Engineering", 2019, 2, "Addison-Wesley", 352, "2nd");

    resources.create("EBOOK", {{"title", "A Tour of C++"}, {"author", "Bjarne Stroustrup"}, {"isbn", "978-0-13-499783-4"}, {"genre", "Programming"},
                               {"publicationYear", "2022"}, {"totalCopies", "5"}, {"fileFormat", "EPUB"}, {"fileSizeMB", "4.2"},
                               {"fileReference", "https://library.example.edu/ebooks/tour-of-cpp"}, {"licenseType", "Perpetual"}});
    resources.create("EBOOK", {{"title", "Artificial Intelligence: A Modern Approach"}, {"author", "Stuart Russell, Peter Norvig"}, {"isbn", "978-0-13-461099-3"},
                               {"genre", "Artificial Intelligence"}, {"publicationYear", "2020"}, {"totalCopies", "3"}, {"fileFormat", "PDF"},
                               {"fileSizeMB", "38.5"}, {"fileReference", "https://library.example.edu/ebooks/aima"}, {"licenseType", "Subscription"}});
    resources.create("EBOOK", {{"title", "Database System Concepts"}, {"author", "Abraham Silberschatz et al."}, {"isbn", "978-0-07-802215-9"},
                               {"genre", "Databases"}, {"publicationYear", "2019"}, {"totalCopies", "4"}, {"fileFormat", "PDF"},
                               {"fileSizeMB", "25"}, {"fileReference", "https://library.example.edu/ebooks/dsc"}, {"licenseType", "Perpetual"}});

    resources.create("JOURNAL", {{"title", "Journal of Systems and Software"}, {"author", "Elsevier"}, {"genre", "Software Engineering"},
                                 {"publicationYear", "2024"}, {"totalCopies", "2"}, {"volume", "210"}, {"issue", "1"},
                                 {"issn", "0164-1212"}, {"academicField", "Software Engineering"}});
    resources.create("JOURNAL", {{"title", "Communications of the ACM"}, {"author", "ACM"}, {"genre", "Computer Science"},
                                 {"publicationYear", "2024"}, {"totalCopies", "1"}, {"volume", "67"}, {"issue", "10"},
                                 {"issn", "0001-0782"}, {"academicField", "Computer Science"}});
    resources.create("JOURNAL", {{"title", "Nature"}, {"author", "Springer Nature"}, {"genre", "Science"},
                                 {"publicationYear", "2024"}, {"totalCopies", "2"}, {"volume", "630"}, {"issue", "8015"},
                                 {"issn", "0028-0836"}, {"academicField", "Multidisciplinary Science"}});

    // ---- members: 6 students (1 suspended, 1 inactive) + 2 faculty ----
    auto member = [&](const char* type, const char* name, const char* email, const char* phone, const char* status, int registeredDaysAgo) {
        members.create(type, {{"fullName", name}, {"email", email}, {"phone", phone}, {"status", status},
                              {"registrationDate", ago(registeredDaysAgo)}});
    };
    member("STUDENT", "Nguyễn Văn A", "nguyen.van.a@student.example.edu", "+84 90 000 0001", "ACTIVE", 400);
    member("STUDENT", "Trần Văn B", "tran.van.b@student.example.edu", "+84 90 000 0002", "ACTIVE", 380);
    member("STUDENT", "Lê Văn C", "le.van.c@student.example.edu", "+84 90 000 0003", "ACTIVE", 300);
    member("STUDENT", "Phạm Thị D", "pham.thi.d@student.example.edu", "+84 90 000 0004", "ACTIVE", 250);
    member("STUDENT", "Hoàng Minh E", "hoang.minh.e@student.example.edu", "+84 90 000 0005", "SUSPENDED", 200);
    member("STUDENT", "Vũ Thị F", "vu.thi.f@student.example.edu", "+84 90 000 0006", "INACTIVE", 600);
    member("FACULTY", "Dr. Lê Quang Huy", "huy.le@faculty.example.edu", "+84 91 000 0001", "ACTIVE", 900);
    member("FACULTY", "Prof. Đặng Thu Hà", "ha.dang@faculty.example.edu", "+84 91 000 0002", "ACTIVE", 1200);

    // ---- loan history (returned, with and without fines) ----
    Loan l1 = loans.borrowResource(2, 6, ago(40));            // Trần Văn B, Harry Potter: returned 6 days late
    loans.returnResource(l1.getLoanId(), std::nullopt, ago(20));
    Loan l2 = loans.borrowResource(3, 2, ago(12));            // on time
    loans.returnResource(l2.getLoanId(), std::nullopt, ago(6));
    Loan l3 = loans.borrowResource(4, 3, ago(33));            // late, fine paid
    loans.returnResource(l3.getLoanId(), std::nullopt, ago(10));
    loans.payFine(l3.getLoanId());
    Loan l4 = loans.borrowResource(1, 7, ago(9));
    loans.returnResource(l4.getLoanId(), std::nullopt, ago(4));

    // ---- active loans ----
    loans.borrowResource(1, 1, ago(30));   // Clean Code: OVERDUE (due 16 days ago)
    loans.borrowResource(1, 2, ago(5));
    loans.borrowResource(2, 1, ago(3));    // second Clean Code copy -> resource now UNAVAILABLE
    loans.borrowResource(3, 4, ago(2));    // Design Patterns (single copy) -> UNAVAILABLE
    loans.borrowResource(4, 3, ago(20));   // Effective Modern C++: OVERDUE (due 6 days ago)
    loans.borrowResource(7, 5, ago(10));   // faculty, Introduction to Algorithms
    loans.borrowResource(7, 9, ago(8));    // faculty, AI e-book
    loans.borrowResource(8, 11, ago(40));  // faculty, journal: OVERDUE (due 10 days ago, 3 grace days)
    loans.borrowResource(2, 8, ago(1));

    // ---- reservations ----
    reservations.reserve(3, 1, ago(2));    // queue for Clean Code: Lê Văn C, then Phạm Thị D
    reservations.reserve(4, 1, ago(1));
    reservations.reserve(1, 4, ago(1));    // queue for Design Patterns: Nguyễn Văn A, then Prof. Hà
    reservations.reserve(8, 4, ago(0));

    // Communications of the ACM (1 copy): borrowed, reserved, returned -> reservation becomes READY
    Loan acm = loans.borrowResource(3, 12, ago(9));
    reservations.reserve(4, 12, ago(4));
    loans.returnResource(acm.getLoanId(), std::nullopt, ago(1));

    library.setDemoData(true);
    library.muteNotifications(false);
    library.notifyChanged();
}
