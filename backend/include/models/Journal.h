#pragma once
#include "models/LibraryResource.h"

class Journal : public LibraryResource {
public:
    static constexpr double LATE_FEE_PER_DAY = 1.00;

    Journal(int id, std::string title, std::string author, std::string genre, int publicationYear,
            int totalCopies, int volume, int issue, std::string issn, std::string academicField);

    std::string getType() const override { return "JOURNAL"; }
    std::string getDescription() const override;
    double getLateFeeRate() const override { return LATE_FEE_PER_DAY; }
    Attributes getExtraAttributes() const override;
    std::string getIdentifier() const override { return issn_; }       // overrides ISBN default
    std::string getIdentifierLabel() const override { return "ISSN"; }

    int getVolume() const { return volume_; }
    int getIssue() const { return issue_; }
    const std::string& getIssn() const { return issn_; }
    const std::string& getAcademicField() const { return academicField_; }

private:
    int volume_;
    int issue_;
    std::string issn_;
    std::string academicField_;
};
