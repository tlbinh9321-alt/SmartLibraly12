#pragma once
#include <optional>
#include <string>
#include <vector>

#include "models/Library.h"
#include "services/ReservationService.h"

struct SearchCriteria {
    std::string query;   // matches title OR author OR identifier OR genre
    std::string title, author, isbn, genre, type;
    std::optional<bool> availableOnly;  // true: only borrowable now, false: only unavailable
    bool hasTextCriteria() const;
};

// Case-insensitive, partial-match search built on std::find_if / std::copy_if
// with small predicate objects.
class SearchService {
public:
    SearchService(const Library& library, const ReservationService& reservations)
        : library_(library), reservations_(reservations) {}

    using Results = std::vector<const LibraryResource*>;
    Results searchByTitle(const std::string& query) const;
    Results searchByAuthor(const std::string& query) const;
    Results searchByISBN(const std::string& query) const;  // ISBN or ISSN
    Results searchByGenre(const std::string& query) const;
    Results searchAll(const std::string& query) const;
    const LibraryResource* findByExactIdentifier(const std::string& identifier) const;  // std::find_if

    Results search(const SearchCriteria& criteria) const;  // throws if no text criterion is given
    Results filter(const SearchCriteria& criteria) const;  // same, but empty criteria = everything

private:
    Results allResources() const;
    const Library& library_;
    const ReservationService& reservations_;
};
