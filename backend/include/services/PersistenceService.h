#pragma once
#include <string>

#include "models/Library.h"
#include "repositories/LibraryFileRepository.h"

class PersistenceService {
public:
    PersistenceService(Library& library, const std::string& dataDirectory)
        : library_(library), repository_(dataDirectory) {}

    void save();
    void load();
    bool hasSavedData() const { return repository_.hasData(); }
    const std::string& lastSavedAt() const { return lastSavedAt_; }
    std::string dataDirectory() const { return repository_.directory().generic_string(); }

private:
    Library& library_;
    LibraryFileRepository repository_;
    std::string lastSavedAt_;
};
