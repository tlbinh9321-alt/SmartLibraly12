#pragma once
#include <string>

#include "models/Library.h"
#include "services/FineService.h"
#include "services/LoanService.h"
#include "services/MemberService.h"
#include "services/PersistenceService.h"
#include "services/ReportService.h"
#include "services/ReservationService.h"
#include "services/ResourceService.h"
#include "services/SearchService.h"

// Composition root of the application layer: one Library plus the focused
// services that operate on it. Neither HTTP nor the UI is involved here, so
// the whole backend can be unit-tested without a server.
class LibraryApp {
public:
    LibraryApp(const std::string& dataDirectory, const std::string& reportsDirectory);
    LibraryApp(const LibraryApp&) = delete;
    LibraryApp& operator=(const LibraryApp&) = delete;

    // Replace everything with clearly-labelled DEMO data (goes through the real services).
    void seedDemoData();

    Library library;
    FineService fines;
    ReservationService reservations;
    ResourceService resources;
    MemberService members;
    LoanService loans;
    SearchService search;
    ReportService reports;
    PersistenceService persistence;
};
