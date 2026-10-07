#pragma once
#include <array>
#include <string>
#include <utility>
#include <vector>

#include "models/Library.h"
#include "services/LoanService.h"

struct InventoryStats {
    std::string generatedOn;  // ISO date
    int totalResources = 0, books = 0, ebooks = 0, journals = 0;
    int totalCopies = 0, availableCopies = 0, borrowedCopies = 0;
    int totalMembers = 0, students = 0, faculty = 0;
    int activeMembers = 0, inactiveMembers = 0, suspendedMembers = 0;
    int totalLoans = 0, activeLoans = 0, overdueLoans = 0, returnedLoans = 0;
    int totalReservations = 0, waitingReservations = 0, readyReservations = 0;
    double outstandingFines = 0.0, collectedFines = 0.0;
    std::array<int, 3> overdueBuckets{{0, 0, 0}};                  // 1-7, 8-14, 15+ days
    std::vector<std::pair<std::string, int>> borrowingTrend;       // loans per day, last 14 days
};

struct ExportedReport {
    std::string path;
    std::string content;
};

class ReportService {
public:
    ReportService(const Library& library, LoanService& loans, std::string reportsDir)
        : library_(library), loans_(loans), reportsDir_(std::move(reportsDir)) {}

    InventoryStats computeStats(const std::string& asOfDate = "");
    std::string buildInventoryReport(const std::string& asOfDate = "");
    std::string buildInventoryCsv(const std::string& asOfDate = "");
    ExportedReport exportInventoryReport(const std::string& asOfDate = "");  // reports/inventory_report.txt
    ExportedReport exportInventoryCsv(const std::string& asOfDate = "");     // reports/inventory_report.csv
    const std::string& reportsDir() const { return reportsDir_; }

private:
    ExportedReport writeFile(const std::string& fileName, const std::string& content) const;

    const Library& library_;
    LoanService& loans_;
    std::string reportsDir_;
};
