#include "services/SearchService.h"

#include <algorithm>
#include <functional>
#include <iterator>

#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

namespace {
using Ptr = const LibraryResource*;
using Results = std::vector<Ptr>;

// Predicate object: "does <field of resource> contain <needle>?" (case-insensitive)
class FieldContains {
public:
    FieldContains(std::string needle, std::function<std::string(const LibraryResource&)> field)
        : needle_(std::move(needle)), field_(std::move(field)) {}
    bool operator()(Ptr resource) const { return StringUtils::containsIgnoreCase(field_(*resource), needle_); }

private:
    std::string needle_;
    std::function<std::string(const LibraryResource&)> field_;
};

bool identifierContains(const LibraryResource& r, const std::string& needle) {
    const std::string wanted = StringUtils::normalizeIdentifier(needle);
    return StringUtils::containsIgnoreCase(r.getIdentifier(), needle) ||
           (!wanted.empty() && StringUtils::normalizeIdentifier(r.getIdentifier()).find(wanted) != std::string::npos);
}

std::string requireQuery(const std::string& query) {
    const std::string trimmed = StringUtils::trim(query);
    if (trimmed.empty()) throw ValidationException("Search query cannot be empty.");
    return trimmed;
}

Results keep(const Results& input, const std::function<bool(Ptr)>& predicate) {
    Results output;
    std::copy_if(input.begin(), input.end(), std::back_inserter(output), predicate);
    return output;
}
}  // namespace

bool SearchCriteria::hasTextCriteria() const {
    return !StringUtils::trim(query).empty() || !StringUtils::trim(title).empty() || !StringUtils::trim(author).empty() ||
           !StringUtils::trim(isbn).empty() || !StringUtils::trim(genre).empty();
}

SearchService::Results SearchService::allResources() const {
    Results all;
    std::transform(library_.resources().begin(), library_.resources().end(), std::back_inserter(all),
                   [](const auto& r) { return r.get(); });
    return all;
}

SearchService::Results SearchService::searchByTitle(const std::string& q) const {
    return keep(allResources(), FieldContains(requireQuery(q), [](const LibraryResource& r) { return r.getTitle(); }));
}
SearchService::Results SearchService::searchByAuthor(const std::string& q) const {
    return keep(allResources(), FieldContains(requireQuery(q), [](const LibraryResource& r) { return r.getAuthor(); }));
}
SearchService::Results SearchService::searchByGenre(const std::string& q) const {
    return keep(allResources(), FieldContains(requireQuery(q), [](const LibraryResource& r) { return r.getGenre(); }));
}
SearchService::Results SearchService::searchByISBN(const std::string& q) const {
    const std::string needle = requireQuery(q);
    return keep(allResources(), [&](Ptr r) { return identifierContains(*r, needle); });
}
SearchService::Results SearchService::searchAll(const std::string& q) const {
    const std::string needle = requireQuery(q);
    return keep(allResources(), [&](Ptr r) {
        return StringUtils::containsIgnoreCase(r->getTitle(), needle) || StringUtils::containsIgnoreCase(r->getAuthor(), needle) ||
               StringUtils::containsIgnoreCase(r->getGenre(), needle) || identifierContains(*r, needle);
    });
}

const LibraryResource* SearchService::findByExactIdentifier(const std::string& identifier) const {
    const std::string wanted = StringUtils::normalizeIdentifier(identifier);
    auto it = std::find_if(library_.resources().begin(), library_.resources().end(), [&](const auto& r) {
        return StringUtils::normalizeIdentifier(r->getIdentifier()) == wanted;
    });
    return (wanted.empty() || it == library_.resources().end()) ? nullptr : it->get();
}

SearchService::Results SearchService::filter(const SearchCriteria& c) const {
    Results results = allResources();
    auto trimmed = [](const std::string& s) { return StringUtils::trim(s); };
    if (!trimmed(c.query).empty()) results = keep(results, [&](Ptr r) {
        const std::string q = trimmed(c.query);
        return StringUtils::containsIgnoreCase(r->getTitle(), q) || StringUtils::containsIgnoreCase(r->getAuthor(), q) ||
               StringUtils::containsIgnoreCase(r->getGenre(), q) || identifierContains(*r, q);
    });
    if (!trimmed(c.title).empty()) results = keep(results, FieldContains(trimmed(c.title), [](const LibraryResource& r) { return r.getTitle(); }));
    if (!trimmed(c.author).empty()) results = keep(results, FieldContains(trimmed(c.author), [](const LibraryResource& r) { return r.getAuthor(); }));
    if (!trimmed(c.genre).empty()) results = keep(results, FieldContains(trimmed(c.genre), [](const LibraryResource& r) { return r.getGenre(); }));
    if (!trimmed(c.isbn).empty()) results = keep(results, [&](Ptr r) { return identifierContains(*r, trimmed(c.isbn)); });
    if (!trimmed(c.type).empty()) results = keep(results, [&](Ptr r) { return StringUtils::equalsIgnoreCase(r->getType(), trimmed(c.type)); });
    if (c.availableOnly.has_value()) {
        const bool wanted = *c.availableOnly;
        results = keep(results, [&](Ptr r) { return (reservations_.effectiveAvailableCopies(*r) > 0) == wanted; });
    }
    return results;
}

SearchService::Results SearchService::search(const SearchCriteria& criteria) const {
    if (!criteria.hasTextCriteria()) throw ValidationException("Search query cannot be empty.");
    return filter(criteria);
}
