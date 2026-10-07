#include "models/EBook.h"

#include <cstdio>

#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

namespace {
std::string formatSize(double mb) {
    char buffer[32];
    std::snprintf(buffer, sizeof buffer, "%.15g", mb);
    return buffer;
}
}  // namespace

EBook::EBook(int id, std::string title, std::string author, std::string isbn, std::string genre,
             int publicationYear, int totalCopies, std::string fileFormat, double fileSizeMB,
             std::string fileReference, std::string licenseType)
    : LibraryResource(id, std::move(title), std::move(author), std::move(isbn), std::move(genre),
                      publicationYear, totalCopies),
      fileFormat_(StringUtils::toUpper(StringUtils::trim(fileFormat))),
      fileSizeMB_(fileSizeMB),
      fileReference_(StringUtils::trim(fileReference)),
      licenseType_(StringUtils::trim(licenseType)) {
    requireIdentifier(getIsbn(), "ISBN");
    if (fileSizeMB_ < 0) throw ValidationException("File size cannot be negative.");
}

std::string EBook::getDescription() const {
    return "Digital " + (fileFormat_.empty() ? std::string("e-book") : fileFormat_) + " edition (" +
           formatSize(fileSizeMB_) + " MB), " + (licenseType_.empty() ? "standard" : licenseType_) +
           " license; each copy is a concurrent license. Late fee $" +
           StringUtils::formatMoney(LATE_FEE_PER_DAY) + "/day.";
}

LibraryResource::Attributes EBook::getExtraAttributes() const {
    return {{"fileFormat", fileFormat_},
            {"fileSizeMB", formatSize(fileSizeMB_)},
            {"fileReference", fileReference_},
            {"licenseType", licenseType_}};
}
