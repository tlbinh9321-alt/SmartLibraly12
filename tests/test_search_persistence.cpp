// 7 Search, 9 Persistence, reports, API envelope
#include <filesystem>
#include <fstream>
#include <sstream>

#include "controllers/LoanController.h"
#include "controllers/ResourceController.h"
#include "controllers/ReservationController.h"
#include "controllers/SearchController.h"
#include "controllers/SystemController.h"
#include "test_framework.h"
#include "test_helpers.h"
#include "utils/Json.h"

namespace {
void addSample(TestApp& t) {
    t->resources.create("BOOK", bookFields("Harry Potter and the Philosopher's Stone", "978-0-7475-3269-9", 2, "J.K. Rowling", "Fantasy"));
    t->resources.create("BOOK", bookFields("Clean Code", "978-0-13-235088-4", 1, "Robert C. Martin", "Software Engineering"));
    t->resources.create("EBOOK", {{"title", "Clean Architecture"}, {"author", "Robert C. Martin"}, {"isbn", "978-0-13-449416-6"}, {"genre", "Software Engineering"},
                                  {"publicationYear", "2017"}, {"totalCopies", "3"}});
    t->resources.create("JOURNAL", {{"title", "Nature"}, {"author", "Springer Nature"}, {"genre", "Science"}, {"publicationYear", "2024"}, {"issn", "0028-0836"}});
}
}  // namespace

TEST(search_is_case_insensitive_and_partial) {
    TestApp t("search1");
    addSample(t);
    auto harry = t->search.searchByTitle("harry");
    CHECK_EQ(harry.size(), size_t(1));
    CHECK_EQ(harry[0]->getTitle(), std::string("Harry Potter and the Philosopher's Stone"));
    CHECK_EQ(t->search.searchByTitle("CLEAN").size(), size_t(2));
    CHECK_EQ(t->search.searchByAuthor("martin").size(), size_t(2));
    CHECK_EQ(t->search.searchByGenre("software").size(), size_t(2));
    CHECK_EQ(t->search.searchByISBN("978-0-7475").size(), size_t(1));
    CHECK_EQ(t->search.searchByISBN("9780132350884").size(), size_t(1));   // hyphens ignored
    CHECK_EQ(t->search.searchByISBN("0028-0836").size(), size_t(1));       // ISSN of a journal
    CHECK_EQ(t->search.searchAll("science").size(), size_t(1));
    CHECK_EQ(t->search.searchByTitle("zzz-nothing").size(), size_t(0));
}

TEST(search_find_if_and_combined_criteria) {
    TestApp t("search2");
    addSample(t);
    const LibraryResource* found = t->search.findByExactIdentifier("978 0 13 235088 4");
    CHECK(found != nullptr);
    CHECK_EQ(found->getTitle(), std::string("Clean Code"));
    CHECK(t->search.findByExactIdentifier("000") == nullptr);
    SearchCriteria c;
    c.author = "martin";
    c.type = "ebook";
    CHECK_EQ(t->search.search(c).size(), size_t(1));
    t->members.create("STUDENT", studentFields("S", "s@x.edu"));
    t->loans.borrowResource(1, 2);  // Clean Code has 1 copy
    SearchCriteria avail;
    avail.query = "clean";
    avail.availableOnly = true;
    CHECK_EQ(t->search.search(avail).size(), size_t(1));  // only the e-book is still available
}

TEST(search_empty_query_is_rejected) {
    TestApp t("search3");
    addSample(t);
    CHECK_THROWS(t->search.searchByTitle("   "), ValidationException, "Search query cannot be empty");
    CHECK_THROWS(t->search.search(SearchCriteria{}), ValidationException, "Search query cannot be empty");
    CHECK_EQ(t->search.filter(SearchCriteria{}).size(), size_t(4));  // filtering without text is allowed
}

TEST(saved_state_can_be_loaded_after_restart) {
    TestApp first("persist1");
    addSample(first);
    first->resources.create("BOOK", bookFields("Pipes | and \\ slashes", "978-0-306-40615-7", 2, "Line\nBreak Author"));
    first->members.create("STUDENT", studentFields("Nguyễn Văn A", "a@x.edu"));
    first->members.create("STUDENT", studentFields("Student B", "b@x.edu"));
    first->members.create("FACULTY", studentFields("Dr. C", "c@x.edu"));
    Loan returned = first->loans.borrowResource(1, 1, "2026-01-01");
    first->loans.returnResource(returned.getLoanId(), std::nullopt, "2026-01-20");   // 5 days late -> $2.50 unpaid
    first->loans.borrowResource(1, 2, "2026-02-01");                                 // Clean Code (1 copy) on loan
    first->reservations.reserve(2, 2, "2026-02-02");
    first->reservations.reserve(3, 2, "2026-02-03");
    first->library.setName("Persisted Library");
    first->persistence.save();
    CHECK(first->persistence.hasSavedData());

    // "restart": a brand-new application object reading the same directory
    LibraryApp second((first.dir / "data").string(), (first.dir / "reports").string());
    CHECK(second.persistence.hasSavedData());
    second.persistence.load();
    CHECK_EQ(second.library.resources().size(), size_t(5));
    CHECK_EQ(second.library.members().size(), size_t(3));
    CHECK_EQ(second.library.loans().size(), size_t(2));
    CHECK_EQ(second.library.getName(), std::string("Persisted Library"));
    CHECK_EQ(second.library.findMember(1)->getFullName(), std::string("Nguyễn Văn A"));
    CHECK_EQ(second.library.findMember(3)->getMemberType(), std::string("FACULTY"));
    CHECK_EQ(second.library.findResource(5)->getTitle(), std::string("Pipes | and \\ slashes"));
    CHECK_EQ(second.library.findResource(5)->getAuthor(), std::string("Line\nBreak Author"));
    CHECK_EQ(second.library.findResource(2)->getAvailableCopies(), 0);
    CHECK_EQ(second.library.findResource(4)->getIdentifier(), std::string("0028-0836"));
    CHECK_NEAR(second.library.findLoan(1)->getFineAmount(), 2.50);
    CHECK(second.library.findLoan(1)->isReturned());
    CHECK_NEAR(second.loans.totalOutstandingFines("2026-03-01") - second.loans.outstandingFine(*second.library.findLoan(2), "2026-03-01"), 2.50);
    // the waiting queue keeps its order
    auto queue = second.reservations.getQueue(2);
    CHECK_EQ(queue.size(), size_t(2));
    CHECK_EQ(queue[0].getMemberId(), 2);
    CHECK_EQ(queue[1].getMemberId(), 3);
    // id counters continue where they stopped
    CHECK_EQ(second.library.ids().loan, 3);
    auto& created = second.resources.create("BOOK", bookFields("After restart", "978-1-86197-876-9"));
    CHECK_EQ(created.getId(), 6);
}

TEST(corrupted_or_missing_files_do_not_destroy_state) {
    TestApp t("persist2");
    addSample(t);
    CHECK(!t->persistence.hasSavedData());
    CHECK_THROWS(t->persistence.load(), PersistenceException, "Missing data file");
    t->persistence.save();
    {   // corrupt the loans file
        std::ofstream out(t.dir / "data" / "loans.txt", std::ios::trunc);
        out << "1|1|1|2026-01-01|2026-01-15||NOT_A_STATUS|0.00|0\n";
    }
    CHECK_THROWS(t->persistence.load(), PersistenceException, "loans.txt line 1");
    CHECK_EQ(t->library.resources().size(), size_t(4));   // in-memory state untouched
}

TEST(change_listener_triggers_autosave) {
    TestApp t("persist3");
    int saves = 0;
    t->library.setChangeListener([&] { ++saves; t->persistence.save(); });
    addSample(t);                                  // 4 creations -> 4 saves
    CHECK_EQ(saves, 4);
    t->members.create("STUDENT", studentFields("S", "s@x.edu"));
    t->loans.borrowResource(1, 1);
    CHECK_EQ(saves, 6);
    CHECK(std::filesystem::exists(t.dir / "data" / "loans.txt"));
    t->library.setChangeListener(nullptr);
}

TEST(inventory_report_contains_real_numbers_and_is_written_to_disk) {
    TestApp t("report");
    addSample(t);
    t->members.create("STUDENT", studentFields("S", "s@x.edu"));
    t->members.create("FACULTY", studentFields("F", "f@x.edu"));
    t->loans.borrowResource(1, 1, "2026-01-01");
    Loan l = t->loans.borrowResource(1, 2, "2026-01-01");
    t->loans.returnResource(l.getLoanId(), std::nullopt, "2026-01-25");   // 10 days late, book -> $5.00
    InventoryStats s = t->reports.computeStats("2026-01-30");
    CHECK_EQ(s.totalResources, 4);
    CHECK_EQ(s.books, 2);
    CHECK_EQ(s.ebooks, 1);
    CHECK_EQ(s.journals, 1);
    CHECK_EQ(s.totalCopies, 2 + 1 + 3 + 1);
    CHECK_EQ(s.borrowedCopies, 1);
    CHECK_EQ(s.students, 1);
    CHECK_EQ(s.faculty, 1);
    CHECK_EQ(s.activeLoans, 1);
    CHECK_EQ(s.overdueLoans, 1);                       // due 2026-01-15, as of 01-30
    CHECK_NEAR(s.outstandingFines, 5.00 + 7.50);       // returned fine + accrued 15 days on the open loan
    ExportedReport txt = t->reports.exportInventoryReport("2026-01-30");
    CHECK(std::filesystem::exists(txt.path));
    CHECK(txt.content.find("SMART LIBRARY INVENTORY REPORT") != std::string::npos);
    CHECK(txt.content.find("30/01/2026") != std::string::npos);
    CHECK(txt.content.find("Books: 2") != std::string::npos);
    CHECK(txt.content.find("$12.50") != std::string::npos);
    ExportedReport csv = t->reports.exportInventoryCsv("2026-01-30");
    CHECK(std::filesystem::exists(csv.path));
    CHECK(csv.content.find("Clean Code") != std::string::npos);
}

namespace {
struct ApiFixture {
    TestApp t;
    Router router;
    ResourceController resources;
    LoanController loans;
    ReservationController reservations;
    SearchController search;
    SystemController system;
    explicit ApiFixture(const std::string& name)
        : t(name), resources(*t), loans(*t), reservations(*t), search(*t), system(*t) {
        resources.registerRoutes(router);
        loans.registerRoutes(router);
        reservations.registerRoutes(router);
        search.registerRoutes(router);
        system.registerRoutes(router);
    }
    HttpResponse call(const std::string& method, const std::string& path, const std::string& body = "") {
        HttpRequest req;
        req.method = method;
        const auto q = path.find('?');
        req.path = path.substr(0, q);
        if (q != std::string::npos) {
            const std::string qs = path.substr(q + 1);
            const auto eq = qs.find('=');
            req.query[qs.substr(0, eq)] = qs.substr(eq + 1);
        }
        req.body = body;
        return router.handle(req);
    }
};
}  // namespace

TEST(api_uses_consistent_success_and_error_envelopes) {
    ApiFixture f("api");
    auto created = f.call("POST", "/api/resources",
                          R"({"type":"BOOK","title":"API Book","author":"A","isbn":"978-0-13-235088-4","genre":"G","publicationYear":2020,"totalCopies":1})");
    CHECK_EQ(created.status, 201);
    Json createdJson = Json::parse(created.body);
    CHECK(createdJson.find("success")->asBool());
    CHECK_EQ(createdJson.find("data")->find("title")->asString(), std::string("API Book"));

    auto dup = f.call("POST", "/api/resources", R"({"type":"BOOK","title":"X","author":"A","isbn":"9780132350884","genre":"G","publicationYear":2020})");
    CHECK_EQ(dup.status, 409);
    CHECK(!Json::parse(dup.body).find("success")->asBool());

    f.t->members.create("STUDENT", studentFields("S", "s@x.edu"));
    auto borrow = f.call("POST", "/api/loans/borrow", R"({"memberId":1,"resourceId":1})");
    CHECK_EQ(borrow.status, 201);
    CHECK_EQ(Json::parse(borrow.body).find("message")->asString(), std::string("Resource borrowed successfully."));
    auto again = f.call("POST", "/api/loans/borrow", R"({"memberId":1,"resourceId":1})");
    CHECK_EQ(again.status, 409);
    CHECK_EQ(f.call("POST", "/api/loans/borrow", R"({"memberId":99,"resourceId":1})").status, 404);
    CHECK_EQ(Json::parse(f.call("POST", "/api/loans/borrow", R"({"memberId":"abc","resourceId":1})").body).find("message")->asString(),
             std::string("Invalid member ID."));
    CHECK_EQ(f.call("POST", "/api/loans/borrow", "{not json").status, 400);
    CHECK_EQ(f.call("GET", "/api/resources/abc").status, 400);
    CHECK_EQ(f.call("GET", "/api/resources/77").status, 404);
    CHECK_EQ(f.call("GET", "/api/nothing").status, 404);
    CHECK_EQ(f.call("PATCH", "/api/resources").status, 405);
    CHECK_EQ(f.call("GET", "/api/search").status, 400);   // empty query
    auto found = Json::parse(f.call("GET", "/api/search?title=api").body);
    CHECK_EQ(static_cast<int>(found.find("data")->find("count")->asNumber()), 1);
    CHECK_EQ(f.call("POST", "/api/data/reset-demo", "{}").status, 400);   // needs explicit confirmation
}

TEST(api_return_reports_fine_and_notifies_next_member) {
    ApiFixture f("api_return");
    f.t->resources.create("BOOK", bookFields("B", "978-0-13-235088-4"));
    f.t->members.create("STUDENT", studentFields("First", "a@x.edu"));
    f.t->members.create("STUDENT", studentFields("Second", "b@x.edu"));
    f.t->loans.borrowResource(1, 1);
    CHECK_EQ(f.call("POST", "/api/reservations", R"({"memberId":2,"resourceId":1})").status, 201);
    auto ret = Json::parse(f.call("POST", "/api/loans/return", R"({"loanId":1,"memberId":1})").body);
    CHECK(ret.find("success")->asBool());
    const Json* notes = ret.find("data")->find("notifications");
    CHECK_EQ(notes->items().size(), size_t(1));
    CHECK(notes->items()[0].asString().find("Next member is ready: Second") != std::string::npos);
}

TEST(demo_data_meets_the_assignment_minimums_and_is_labelled) {
    TestApp t("demo");
    t->seedDemoData();
    InventoryStats s = t->reports.computeStats();
    CHECK(t->library.isDemoData());
    CHECK(s.totalResources >= 10);
    CHECK(s.students >= 5);
    CHECK(s.faculty >= 2);
    CHECK(s.totalLoans >= 3);
    CHECK(s.overdueLoans >= 1);
    CHECK(s.waitingReservations >= 1);
    bool someUnavailable = false;
    for (const auto& r : t->library.resources()) if (t->reservations.effectiveAvailableCopies(*r) == 0) someUnavailable = true;
    CHECK(someUnavailable);
    CHECK(t->reports.buildInventoryReport().find("DEMO") != std::string::npos);
    // demo data survives a save/load round trip too
    t->persistence.save();
    LibraryApp again((t.dir / "data").string(), (t.dir / "reports").string());
    again.persistence.load();
    CHECK(again.library.isDemoData());
    CHECK_EQ(again.library.resources().size(), t->library.resources().size());
    CHECK_EQ(again.library.allReservations().size(), t->library.allReservations().size());
}
