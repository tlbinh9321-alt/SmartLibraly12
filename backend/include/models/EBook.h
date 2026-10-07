#pragma once
#include "models/LibraryResource.h"

class EBook : public LibraryResource {
public:
    static constexpr double LATE_FEE_PER_DAY = 0.20;

    EBook(int id, std::string title, std::string author, std::string isbn, std::string genre,
          int publicationYear, int totalCopies, std::string fileFormat, double fileSizeMB,
          std::string fileReference, std::string licenseType);

    std::string getType() const override { return "EBOOK"; }
    std::string getDescription() const override;
    double getLateFeeRate() const override { return LATE_FEE_PER_DAY; }
    Attributes getExtraAttributes() const override;

    const std::string& getFileFormat() const { return fileFormat_; }
    double getFileSizeMB() const { return fileSizeMB_; }
    const std::string& getFileReference() const { return fileReference_; }
    const std::string& getLicenseType() const { return licenseType_; }

private:
    std::string fileFormat_;
    double fileSizeMB_;
    std::string fileReference_;
    std::string licenseType_;
};
