#pragma once
#include <filesystem>
#include <string>

#include "models/Library.h"

// Reads/writes the whole Library as plain text files:
//   resources.txt  members.txt  loans.txt  reservations.txt  settings.txt
// Format choice: pipe-delimited text (one record per line). It is human
// readable, diff-friendly, needs no third-party parser and is easy to explain.
// save(): every file is written to *.tmp first and then renamed, so a crash
// cannot leave a half-written file. load(): parsed into a scratch Library and
// only swapped in when everything is valid (all-or-nothing).
class LibraryFileRepository {
public:
    explicit LibraryFileRepository(std::filesystem::path dataDirectory) : dir_(std::move(dataDirectory)) {}

    void save(const Library& library) const;
    void load(Library& library) const;  // throws PersistenceException
    bool hasData() const;
    const std::filesystem::path& directory() const { return dir_; }
    static const char* const* fileNames();  // NULL-terminated list

private:
    std::filesystem::path dir_;
};
