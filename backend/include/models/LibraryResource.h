#pragma once
#include <string>
#include <utility>
#include <vector>

// ABSTRACT BASE CLASS for everything the library lends.
// Encapsulation: all state is private; copies change only through
// borrowCopy()/returnCopy()/setTotalCopies()/restoreAvailableCopies(), which
// protect the invariant 0 <= availableCopies <= totalCopies.
class LibraryResource {
public:
    using Attributes = std::vector<std::pair<std::string, std::string>>;

    LibraryResource(int id, std::string title, std::string author, std::string isbn,
                    std::string genre, int publicationYear, int totalCopies);
    virtual ~LibraryResource() = default;

    // ---- polymorphic interface (overridden by Book, EBook, Journal) ----
    virtual std::string getType() const = 0;         // "BOOK" | "EBOOK" | "JOURNAL"
    virtual std::string getDescription() const = 0;
    virtual double getLateFeeRate() const = 0;       // base fine per overdue day
    virtual Attributes getExtraAttributes() const = 0;  // subclass fields, used by JSON + file I/O
    virtual std::string getIdentifier() const { return isbn_; }          // Journal -> ISSN
    virtual std::string getIdentifierLabel() const { return "ISBN"; }

    // ---- common behaviour ----
    int getId() const { return id_; }
    const std::string& getTitle() const { return title_; }
    const std::string& getAuthor() const { return author_; }
    const std::string& getIsbn() const { return isbn_; }
    const std::string& getGenre() const { return genre_; }
    int getPublicationYear() const { return publicationYear_; }
    int getTotalCopies() const { return totalCopies_; }
    int getAvailableCopies() const { return availableCopies_; }
    int getBorrowedCopies() const { return totalCopies_ - availableCopies_; }
    bool isAvailable() const { return availableCopies_ > 0; }

    void borrowCopy();                         // throws if no copy is free
    void returnCopy();                         // throws if every copy is already home
    void setTotalCopies(int totalCopies);      // keeps borrowed copies consistent
    void restoreAvailableCopies(int available);  // used when loading saved state / updating

protected:
    void requireIdentifier(const std::string& value, const std::string& label) const;

private:
    int id_;
    std::string title_;
    std::string author_;
    std::string isbn_;
    std::string genre_;
    int publicationYear_;
    int totalCopies_;
    int availableCopies_;
};
