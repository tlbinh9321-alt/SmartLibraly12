#pragma once
#include "models/Library.h"
#include "services/LibraryApp.h"
#include "utils/Json.h"

// Converts domain objects to JSON for API responses (read-only, no rules here).
class JsonMapper {
public:
    explicit JsonMapper(LibraryApp& app) : app_(app) {}

    Json resource(const LibraryResource& resource) const;
    Json member(const Member& member) const;
    Json loan(const Loan& loan) const;
    Json reservation(const Reservation& reservation) const;
    Json queueOf(int resourceId) const;          // WAITING reservations with 1-based positions
    Json reservationQueues() const;              // every resource that is unavailable or has a queue
    Json stats(const InventoryStats& stats) const;
    Json loanList(const std::vector<Loan>& loans, size_t limit = 0) const;  // newest first

private:
    LibraryApp& app_;
};
