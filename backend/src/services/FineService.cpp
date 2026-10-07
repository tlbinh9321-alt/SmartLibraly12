#include "services/FineService.h"

#include "models/FinePolicy.h"

double FineService::calculateFine(const Member& member, const LibraryResource& resource, int overdueDays) const {
    return member.createFinePolicy(resource.getLateFeeRate())->calculateFine(overdueDays);
}

std::string FineService::describePolicy(const Member& member, const LibraryResource& resource) const {
    return member.createFinePolicy(resource.getLateFeeRate())->describe();
}
