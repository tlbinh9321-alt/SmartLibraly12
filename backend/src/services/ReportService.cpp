#include "services/ReportService.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "utils/DateUtils.h"
#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

namespace {
std::string csvCell(const std::string& value) {
    if (value.find_first_of(",\"\n\r") == std::string::npos) return value;
    std::string out = "\"";
    for (char c : value) out += (c == '"') ? std::string("\"\"") : std::string(1, c);
    return out + "\"";
}
}  // namespace

InventoryStats ReportService::computeStats(const std::string& asOfDate) {
    const std::string today = asOfDate.empty() ? DateUtils::today() : asOfDate;
    InventoryStats s;
    s.generatedOn = today;

    for (const auto& resource : library_.resources()) {
        ++s.totalResources;
        const std::string type = resource->getType();
        if (type == "BOOK") ++s.books;
        else if (type == "EBOOK") ++s.ebooks;
        else if (type == "JOURNAL") ++s.journals;
        s.totalCopies += resource->getTotalCopies();
        s.availableCopies += resource->getAvailableCopies();
    }
    s.borrowedCopies = s.totalCopies - s.availableCopies;

    for (const auto& member : library_.members()) {
        ++s.totalMembers;
        if (member->getMemberType() == "STUDENT") ++s.students; else ++s.faculty;
        switch (member->getStatus()) {
            case MemberStatus::ACTIVE: ++s.activeMembers; break;
            case MemberStatus::INACTIVE: ++s.inactiveMembers; break;
            case MemberStatus::SUSPENDED: ++s.suspendedMembers; break;
        }
    }

    const std::vector<Loan> loans = loans_.getAllLoans(today);  // also refreshes OVERDUE flags
    s.totalLoans = static_cast<int>(loans.size());
    for (const Loan& loan : loans) {
        if (loan.isReturned()) { ++s.returnedLoans; continue; }
        ++s.activeLoans;
        if (loan.getStatus() == LoanStatus::OVERDUE) {
            ++s.overdueLoans;
            const int days = loans_.calculateOverdueDays(loan, today);
            ++s.overdueBuckets[days <= 7 ? 0 : (days <= 14 ? 1 : 2)];
        }
    }

    for (const Reservation& r : library_.allReservations()) {
        ++s.totalReservations;
        if (r.getStatus() == ReservationStatus::WAITING) ++s.waitingReservations;
        if (r.getStatus() == ReservationStatus::READY) ++s.readyReservations;
    }

    s.outstandingFines = StringUtils::round2(loans_.totalOutstandingFines(today));
    s.collectedFines = StringUtils::round2(loans_.totalCollectedFines());

    const int todayNumber = DateUtils::toDayNumber(today);
    for (int offset = 13; offset >= 0; --offset) {
        const std::string day = DateUtils::fromDayNumber(todayNumber - offset);
        int count = 0;
        for (const Loan& loan : loans) if (loan.getBorrowDate() == day) ++count;
        s.borrowingTrend.emplace_back(day, count);
    }
    return s;
}

std::string ReportService::buildInventoryReport(const std::string& asOfDate) {
    const InventoryStats s = computeStats(asOfDate);
    const std::string bar(37, '=');
    std::ostringstream out;
    out << bar << "\nSMART LIBRARY INVENTORY REPORT\n" << bar << "\n\n";
    if (library_.isDemoData()) out << "NOTE: the library currently holds DEMO / SAMPLE data.\n\n";
    out << "Generated:\n" << DateUtils::toDayMonthYear(s.generatedOn) << "\n\n";
    out << "Resources:\nBooks: " << s.books << "\nEBooks: " << s.ebooks << "\nJournals: " << s.journals
        << "\n\nTotal resources: " << s.totalResources << "\n\n";
    out << "Copies:\nTotal copies: " << s.totalCopies << "\nAvailable copies: " << s.availableCopies
        << "\nBorrowed copies: " << s.borrowedCopies << "\n\n";
    out << "Members:\nStudents: " << s.students << "\nFaculty: " << s.faculty << "\nTotal members: " << s.totalMembers
        << "\n(Active: " << s.activeMembers << ", Inactive: " << s.inactiveMembers << ", Suspended: " << s.suspendedMembers << ")\n\n";
    out << "Active loans: " << s.activeLoans << "\nOverdue loans: " << s.overdueLoans << "\nReturned loans: " << s.returnedLoans << "\n\n";
    out << "Reservations:\nWaiting: " << s.waitingReservations << "\nReady for pickup: " << s.readyReservations
        << "\nTotal recorded: " << s.totalReservations << "\n\n";
    out << "Outstanding fines:\n$" << StringUtils::formatMoney(s.outstandingFines) << "\n\n";
    out << "Fines collected:\n$" << StringUtils::formatMoney(s.collectedFines) << "\n\n" << bar << "\n";
    return out.str();
}

std::string ReportService::buildInventoryCsv(const std::string& asOfDate) {
    const InventoryStats s = computeStats(asOfDate);
    std::ostringstream out;
    out << "metric,value\n"
        << "generated_on," << s.generatedOn << "\n"
        << "data_is_demo," << (library_.isDemoData() ? "yes" : "no") << "\n"
        << "total_resources," << s.totalResources << "\nbooks," << s.books << "\nebooks," << s.ebooks << "\njournals," << s.journals << "\n"
        << "total_copies," << s.totalCopies << "\navailable_copies," << s.availableCopies << "\nborrowed_copies," << s.borrowedCopies << "\n"
        << "students," << s.students << "\nfaculty," << s.faculty << "\n"
        << "active_loans," << s.activeLoans << "\noverdue_loans," << s.overdueLoans << "\n"
        << "waiting_reservations," << s.waitingReservations << "\nready_reservations," << s.readyReservations << "\n"
        << "outstanding_fines," << StringUtils::formatMoney(s.outstandingFines) << "\n\n"
        << "id,type,title,author,identifier,genre,year,total_copies,available_copies,borrowed_copies\n";
    for (const auto& r : library_.resources())
        out << r->getId() << ',' << r->getType() << ',' << csvCell(r->getTitle()) << ',' << csvCell(r->getAuthor()) << ','
            << csvCell(r->getIdentifier()) << ',' << csvCell(r->getGenre()) << ',' << r->getPublicationYear() << ','
            << r->getTotalCopies() << ',' << r->getAvailableCopies() << ',' << r->getBorrowedCopies() << '\n';
    return out.str();
}

ExportedReport ReportService::writeFile(const std::string& fileName, const std::string& content) const {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::create_directories(reportsDir_, ec);
    const fs::path path = fs::path(reportsDir_) / fileName;
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) throw PersistenceException("Cannot write report file: " + path.string());
    file << content;
    file.close();
    if (!file) throw PersistenceException("Failed while writing report file: " + path.string());
    return {path.generic_string(), content};
}

ExportedReport ReportService::exportInventoryReport(const std::string& asOfDate) {
    return writeFile("inventory_report.txt", buildInventoryReport(asOfDate));
}
ExportedReport ReportService::exportInventoryCsv(const std::string& asOfDate) {
    return writeFile("inventory_report.csv", buildInventoryCsv(asOfDate));
}
