#include "models/LibraryResource.h"

#include <cctype>

#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

namespace {
// ISBN-10 or ISBN-13 (hyphens/spaces allowed); checksum is not verified.
bool looksLikeIsbn(const std::string& text) {
    std::string digits;
    for (unsigned char c : text) {
        if (c == '-' || c == ' ') continue;
        digits += static_cast<char>(c);
    }
    if (digits.size() == 13) {
        for (unsigned char c : digits) if (!std::isdigit(c)) return false;
        return true;
    }
    if (digits.size() == 10) {
        for (size_t i = 0; i < 9; ++i) if (!std::isdigit(static_cast<unsigned char>(digits[i]))) return false;
        const char last = digits[9];
        return std::isdigit(static_cast<unsigned char>(last)) || last == 'X' || last == 'x';
    }
    return false;
}
}  // namespace

LibraryResource::LibraryResource(int id, std::string title, std::string author, std::string isbn,
                                 std::string genre, int publicationYear, int totalCopies)
    : id_(id),
      title_(StringUtils::trim(title)),
      author_(StringUtils::trim(author)),
      isbn_(StringUtils::trim(isbn)),
      genre_(StringUtils::trim(genre)),
      publicationYear_(publicationYear),
      totalCopies_(totalCopies),
      availableCopies_(totalCopies) {
    if (title_.empty()) throw ValidationException("Title is required.");
    if (author_.empty()) throw ValidationException("Author is required.");
    if (genre_.empty()) throw ValidationException("Genre is required.");
    if (!isbn_.empty() && !looksLikeIsbn(isbn_))
        throw ValidationException("Invalid ISBN '" + isbn_ + "' (expected 10 or 13 digits).");
    if (publicationYear_ < 1000 || publicationYear_ > 2100)
        throw ValidationException("Publication year must be between 1000 and 2100.");
    if (totalCopies_ < 0) throw ValidationException("Total copies cannot be negative.");
}

void LibraryResource::borrowCopy() {
    if (availableCopies_ <= 0) throw ValidationException("Resource is currently unavailable.");
    --availableCopies_;
}

void LibraryResource::returnCopy() {
    if (availableCopies_ >= totalCopies_) throw ValidationException("All copies are already in the library.");
    ++availableCopies_;
}

void LibraryResource::setTotalCopies(int totalCopies) {
    if (totalCopies < 0) throw ValidationException("Total copies cannot be negative.");
    const int borrowed = getBorrowedCopies();
    if (totalCopies < borrowed)
        throw ValidationException("Total copies cannot be lower than the " + std::to_string(borrowed) +
                                  " copies currently on loan.");
    totalCopies_ = totalCopies;
    availableCopies_ = totalCopies - borrowed;
}

void LibraryResource::restoreAvailableCopies(int available) {
    if (available < 0 || available > totalCopies_)
        throw ValidationException("Available copies must be between 0 and the total number of copies.");
    availableCopies_ = available;
}

void LibraryResource::requireIdentifier(const std::string& value, const std::string& label) const {
    if (value.empty()) throw ValidationException(label + " is required.");
}
