#include "services/ResourceService.h"

#include <algorithm>

#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

void ResourceService::ensureUniqueIdentifier(const LibraryResource& candidate) const {
    const std::string wanted = StringUtils::normalizeIdentifier(candidate.getIdentifier());
    if (wanted.empty()) return;
    for (const auto& existing : library_.resources())
        if (existing->getId() != candidate.getId() &&
            StringUtils::normalizeIdentifier(existing->getIdentifier()) == wanted)
            throw ConflictException("Duplicate " + candidate.getIdentifierLabel() + ": " + candidate.getIdentifier() +
                                    " already belongs to '" + existing->getTitle() + "'.");
}

LibraryResource& ResourceService::create(const std::string& type, const FieldMap& fields) {
    auto resource = ResourceFactory::create(type, library_.ids().resource, fields);
    ensureUniqueIdentifier(*resource);
    ++library_.ids().resource;  // only burn an id once validation has passed
    library_.resources().push_back(std::move(resource));
    library_.notifyChanged();
    return *library_.resources().back();
}

const LibraryResource& ResourceService::getById(int id) const {
    const LibraryResource* resource = library_.findResource(id);
    if (!resource) throw NotFoundException("Resource not found.");
    return *resource;
}

std::vector<const LibraryResource*> ResourceService::getAll() const {
    std::vector<const LibraryResource*> all;
    std::transform(library_.resources().begin(), library_.resources().end(), std::back_inserter(all),
                   [](const auto& r) { return r.get(); });
    return all;
}

LibraryResource& ResourceService::update(int id, const FieldMap& changes) {
    LibraryResource* current = library_.findResource(id);
    if (!current) throw NotFoundException("Resource not found.");

    const std::string type = current->getType();
    auto requested = changes.find("type");
    if (requested != changes.end() && !StringUtils::equalsIgnoreCase(requested->second, type))
        throw ValidationException("Resource type cannot be changed.");

    FieldMap merged = ResourceFactory::toFields(*current);
    for (const auto& change : changes) merged[change.first] = change.second;

    auto replacement = ResourceFactory::create(type, id, merged);
    const int borrowed = current->getBorrowedCopies();
    if (replacement->getTotalCopies() < borrowed)
        throw ValidationException("Total copies cannot be lower than the " + std::to_string(borrowed) +
                                  " copies currently on loan.");
    replacement->restoreAvailableCopies(replacement->getTotalCopies() - borrowed);
    ensureUniqueIdentifier(*replacement);

    for (auto& slot : library_.resources())
        if (slot->getId() == id) slot = std::move(replacement);
    reservations_.fulfillQueue(id);  // extra copies may satisfy waiting members
    library_.notifyChanged();
    return *library_.findResource(id);
}

void ResourceService::remove(int id) {
    if (!library_.findResource(id)) throw NotFoundException("Resource not found.");
    for (const Loan& loan : library_.loans())
        if (loan.isActive() && loan.getResourceId() == id)
            throw ConflictException("Cannot delete a resource that is currently on loan.");
    for (const Reservation& r : library_.allReservations())
        if (r.isOpen() && r.getResourceId() == id)
            throw ConflictException("Cannot delete a resource that has open reservations. Cancel them first.");
    auto& list = library_.resources();
    list.erase(std::remove_if(list.begin(), list.end(), [id](const auto& r) { return r->getId() == id; }), list.end());
    library_.notifyChanged();
}
