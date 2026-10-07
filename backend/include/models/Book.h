#pragma once
#include "models/LibraryResource.h"

class Book : public LibraryResource {
public:
    static constexpr double LATE_FEE_PER_DAY = 0.50;

    Book(int id, std::string title, std::string author, std::string isbn, std::string genre,
         int publicationYear, int totalCopies, std::string publisher, int pageCount, std::string edition);

    std::string getType() const override { return "BOOK"; }
    std::string getDescription() const override;
    double getLateFeeRate() const override { return LATE_FEE_PER_DAY; }
    Attributes getExtraAttributes() const override;

    const std::string& getPublisher() const { return publisher_; }
    int getPageCount() const { return pageCount_; }
    const std::string& getEdition() const { return edition_; }

private:
    std::string publisher_;
    int pageCount_;
    std::string edition_;
};
