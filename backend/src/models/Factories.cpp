#include "models/Factories.h"

#include <cerrno>
#include <cstdlib>

#include "models/Book.h"
#include "models/EBook.h"
#include "models/Journal.h"
#include "utils/DateUtils.h"
#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

namespace {
std::string text(const FieldMap& f, const std::string& key, const std::string& fallback = "") {
    auto it = f.find(key);
    return it == f.end() ? fallback : StringUtils::trim(it->second);
}

int integer(const FieldMap& f, const std::string& key, const std::string& label, bool required, int fallback = 0) {
    const std::string value = text(f, key);
    if (value.empty()) {
        if (required) throw ValidationException(label + " is required.");
        return fallback;
    }
    char* end = nullptr;
    errno = 0;
    const long parsed = std::strtol(value.c_str(), &end, 10);
    if (end == value.c_str() || *end != '\0' || errno == ERANGE || parsed > 1000000000L || parsed < -1000000000L)
        throw ValidationException(label + " must be a whole number.");
    return static_cast<int>(parsed);
}

double decimal(const FieldMap& f, const std::string& key, const std::string& label, double fallback) {
    const std::string value = text(f, key);
    if (value.empty()) return fallback;
    char* end = nullptr;
    const double parsed = std::strtod(value.c_str(), &end);
    if (end == value.c_str() || *end != '\0') throw ValidationException(label + " must be a number.");
    return parsed;
}
}  // namespace

std::unique_ptr<LibraryResource> ResourceFactory::create(const std::string& type, int id, const FieldMap& f) {
    const std::string kind = StringUtils::toUpper(StringUtils::trim(type));
    const std::string title = text(f, "title"), author = text(f, "author"), genre = text(f, "genre");
    const int year = integer(f, "publicationYear", "Publication year", true);
    const int copies = integer(f, "totalCopies", "Total copies", false, 1);

    if (kind == "BOOK")
        return std::make_unique<Book>(id, title, author, text(f, "isbn"), genre, year, copies,
                                      text(f, "publisher"), integer(f, "pageCount", "Page count", false),
                                      text(f, "edition"));
    if (kind == "EBOOK")
        return std::make_unique<EBook>(id, title, author, text(f, "isbn"), genre, year, copies,
                                       text(f, "fileFormat", "PDF"), decimal(f, "fileSizeMB", "File size", 0.0),
                                       text(f, "fileReference"), text(f, "licenseType", "Perpetual"));
    if (kind == "JOURNAL")
        return std::make_unique<Journal>(id, title, author, genre, year, copies,
                                         integer(f, "volume", "Volume", false, 1), integer(f, "issue", "Issue", false, 1),
                                         text(f, "issn"), text(f, "academicField"));
    throw ValidationException("Unknown resource type '" + type + "' (expected BOOK, EBOOK or JOURNAL).");
}

FieldMap ResourceFactory::toFields(const LibraryResource& r) {
    FieldMap f{{"title", r.getTitle()}, {"author", r.getAuthor()}, {"isbn", r.getIsbn()}, {"genre", r.getGenre()},
               {"publicationYear", std::to_string(r.getPublicationYear())},
               {"totalCopies", std::to_string(r.getTotalCopies())}};
    for (const auto& attribute : r.getExtraAttributes()) f[attribute.first] = attribute.second;
    return f;
}

std::unique_ptr<Member> MemberFactory::create(const std::string& type, int id, const FieldMap& f) {
    const std::string kind = StringUtils::toUpper(StringUtils::trim(type));
    const MemberStatus status = parseMemberStatus(text(f, "status", "ACTIVE"));
    const std::string registered = text(f, "registrationDate", DateUtils::today());
    if (kind == "STUDENT")
        return std::make_unique<StudentMember>(id, text(f, "fullName"), text(f, "email"), text(f, "phone"), status, registered);
    if (kind == "FACULTY")
        return std::make_unique<FacultyMember>(id, text(f, "fullName"), text(f, "email"), text(f, "phone"), status, registered);
    throw ValidationException("Unknown member type '" + type + "' (expected STUDENT or FACULTY).");
}

FieldMap MemberFactory::toFields(const Member& m) {
    return {{"fullName", m.getFullName()}, {"email", m.getEmail()}, {"phone", m.getPhone()},
            {"status", toString(m.getStatus())}, {"registrationDate", m.getRegistrationDate()}};
}
