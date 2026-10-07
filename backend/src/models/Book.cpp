#include "models/Book.h"

#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

Book::Book(int id, std::string title, std::string author, std::string isbn, std::string genre,
           int publicationYear, int totalCopies, std::string publisher, int pageCount, std::string edition)
    : LibraryResource(id, std::move(title), std::move(author), std::move(isbn), std::move(genre),
                      publicationYear, totalCopies),
      publisher_(StringUtils::trim(publisher)),
      pageCount_(pageCount),
      edition_(StringUtils::trim(edition)) {
    requireIdentifier(getIsbn(), "ISBN");
    if (pageCount_ < 0) throw ValidationException("Page count cannot be negative.");
}

std::string Book::getDescription() const {
    std::string text = "Printed book";
    if (!publisher_.empty()) text += " published by " + publisher_;
    if (!edition_.empty()) text += " (" + edition_ + " edition)";
    if (pageCount_ > 0) text += ", " + std::to_string(pageCount_) + " pages";
    return text + ". Borrowed physically; late fee $" + StringUtils::formatMoney(LATE_FEE_PER_DAY) + "/day.";
}

LibraryResource::Attributes Book::getExtraAttributes() const {
    return {{"publisher", publisher_}, {"pageCount", std::to_string(pageCount_)}, {"edition", edition_}};
}
