#pragma once
#include <string>

#include "models/LibraryResource.h"
#include "models/Member.h"

// Single place where fines are computed. It composes the two polymorphic
// sources of truth: resource.getLateFeeRate() and member.createFinePolicy().
class FineService {
public:
    double calculateFine(const Member& member, const LibraryResource& resource, int overdueDays) const;
    std::string describePolicy(const Member& member, const LibraryResource& resource) const;
};
