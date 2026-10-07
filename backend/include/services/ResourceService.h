#pragma once
#include <vector>

#include "models/Factories.h"
#include "models/Library.h"
#include "services/ReservationService.h"

class ResourceService {
public:
    ResourceService(Library& library, ReservationService& reservations)
        : library_(library), reservations_(reservations) {}

    LibraryResource& create(const std::string& type, const FieldMap& fields);
    const LibraryResource& getById(int id) const;
    std::vector<const LibraryResource*> getAll() const;
    LibraryResource& update(int id, const FieldMap& changes);  // partial update, type is fixed
    void remove(int id);

private:
    void ensureUniqueIdentifier(const LibraryResource& candidate) const;

    Library& library_;
    ReservationService& reservations_;
};
