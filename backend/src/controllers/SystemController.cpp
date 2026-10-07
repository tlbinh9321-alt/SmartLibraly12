#include "controllers/SystemController.h"

#include <filesystem>

#include "controllers/ApiSupport.h"
#include "models/Book.h"
#include "models/EBook.h"
#include "models/FinePolicy.h"
#include "models/Journal.h"
#include "repositories/LibraryFileRepository.h"
#include "utils/DateUtils.h"
#include "utils/StringUtils.h"

namespace {
Json counts(const Library& library) {
    return Json::object().set("resources", static_cast<int>(library.resources().size()))
        .set("members", static_cast<int>(library.members().size()))
        .set("loans", static_cast<int>(library.loans().size()))
        .set("reservations", static_cast<int>(library.allReservations().size()));
}
}  // namespace

void SystemController::registerRoutes(Router& router) {
    router.add("GET", "/api/health", [this](const HttpRequest&) {
        return Api::guarded([&] { Json data = counts(app_.library);
            data.set("demoData", app_.library.isDemoData()).set("libraryName", app_.library.getName());
            return Api::ok("Smart Library backend is running.", data); });
    });

    router.add("GET", "/api/dashboard", [this](const HttpRequest&) {
        return Api::guarded([&] {
            const InventoryStats stats = app_.reports.computeStats();
            const std::vector<Loan> loans = app_.loans.getAllLoans();
            Json data = Json::object();
            data.set("libraryName", app_.library.getName()).set("demoData", app_.library.isDemoData())
                .set("stats", mapper_.stats(stats))
                .set("recentLoans", mapper_.loanList(loans, 8))
                .set("overdueLoans", mapper_.loanList(app_.loans.getOverdueLoans()))
                .set("queues", mapper_.reservationQueues());
            return Api::ok("Dashboard data retrieved.", data);
        });
    });

    router.add("GET", "/api/reports/analytics", [this](const HttpRequest&) {
        return Api::guarded([&] { return Api::ok("Analytics retrieved.", mapper_.stats(app_.reports.computeStats())); });
    });

    router.add("GET", "/api/reports/inventory", [this](const HttpRequest&) {
        return Api::guarded([&] {
            const ExportedReport report = app_.reports.exportInventoryReport();
            Json data = Json::object();
            data.set("fileName", "inventory_report.txt").set("path", report.path).set("content", report.content);
            return Api::ok("Inventory report generated.", data);
        });
    });

    router.add("GET", "/api/reports/csv", [this](const HttpRequest&) {
        return Api::guarded([&] {
            const ExportedReport report = app_.reports.exportInventoryCsv();
            Json data = Json::object();
            data.set("fileName", "inventory_report.csv").set("path", report.path).set("content", report.content);
            return Api::ok("Inventory CSV generated.", data);
        });
    });

    router.add("POST", "/api/data/save", [this](const HttpRequest&) {
        return Api::guarded([&] {
            app_.persistence.save();
            Json data = Json::object();
            data.set("savedAt", app_.persistence.lastSavedAt()).set("directory", app_.persistence.dataDirectory());
            return Api::ok("System state saved.", data);
        });
    });

    router.add("POST", "/api/data/load", [this](const HttpRequest&) {
        return Api::guarded([&] {
            app_.persistence.load();
            return Api::ok("System state loaded from files.", counts(app_.library));
        });
    });

    router.add("POST", "/api/data/reset-demo", [this](const HttpRequest& req) {
        return Api::guarded([&] {
            const Json body = Api::parseBody(req);
            const Json* confirm = body.find("confirm");
            if (!confirm || !confirm->isBool() || !confirm->asBool())
                throw ValidationException("Reset requires confirmation (send {\"confirm\": true}).");
            app_.seedDemoData();  // replaces ALL data and saves once
            return Api::ok("Demo data restored. All previous data was replaced.", counts(app_.library));
        });
    });

    router.add("GET", "/api/settings", [this](const HttpRequest&) {
        return Api::guarded([&] {
            namespace fs = std::filesystem;
            Json borrowing = Json::array();
            borrowing.push(Json::object().set("memberType", "STUDENT").set("borrowLimit", StudentMember::BORROW_LIMIT).set("loanDays", StudentMember::LOAN_DAYS));
            borrowing.push(Json::object().set("memberType", "FACULTY").set("borrowLimit", FacultyMember::BORROW_LIMIT).set("loanDays", FacultyMember::LOAN_DAYS));
            Json rates = Json::array();
            rates.push(Json::object().set("type", "BOOK").set("ratePerDay", Book::LATE_FEE_PER_DAY));
            rates.push(Json::object().set("type", "EBOOK").set("ratePerDay", EBook::LATE_FEE_PER_DAY));
            rates.push(Json::object().set("type", "JOURNAL").set("ratePerDay", Journal::LATE_FEE_PER_DAY));
            Json rules = Json::array();
            rules.push(Json::object().set("memberType", "STUDENT").set("rateFactor", StudentFinePolicy::RATE_FACTOR).set("graceDays", 0)
                           .set("description", "Full resource rate from the first overdue day"));
            rules.push(Json::object().set("memberType", "FACULTY").set("rateFactor", FacultyFinePolicy::RATE_FACTOR).set("graceDays", FacultyFinePolicy::GRACE_DAYS)
                           .set("description", "Grace period, then a discounted rate"));
            Json files = Json::array();
            const fs::path dir = app_.persistence.dataDirectory();
            for (const char* const* name = LibraryFileRepository::fileNames(); *name; ++name) {
                std::error_code ec;
                const bool exists = fs::exists(dir / *name, ec);
                files.push(Json::object().set("name", *name).set("exists", exists)
                               .set("bytes", exists ? static_cast<double>(fs::file_size(dir / *name, ec)) : 0.0));
            }
            Json data = Json::object();
            data.set("libraryName", app_.library.getName()).set("demoData", app_.library.isDemoData())
                .set("borrowingPolicies", borrowing)
                .set("fineRules", Json::object().set("resourceRates", rates).set("memberRules", rules).set("formula",
                     "fine = billable overdue days x resource rate x member rate factor"))
                .set("persistence", Json::object().set("directory", app_.persistence.dataDirectory()).set("files", files)
                                        .set("lastSavedAt", app_.persistence.lastSavedAt()).set("format", "Pipe-delimited text, written atomically (*.tmp then rename)")
                                        .set("autoSave", true))
                .set("status", counts(app_.library).set("today", DateUtils::today()).set("reportsDirectory", app_.reports.reportsDir()));
            return Api::ok("Settings retrieved.", data);
        });
    });

    router.add("PUT", "/api/settings", [this](const HttpRequest& req) {
        return Api::guarded([&] {
            const Json body = Api::parseBody(req);
            const Json* name = body.find("libraryName");
            if (!name || !name->isString()) throw ValidationException("libraryName is required.");
            app_.library.setName(name->asString());
            app_.library.notifyChanged();
            return Api::ok("Settings saved.", Json::object().set("libraryName", app_.library.getName()));
        });
    });
}
