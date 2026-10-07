#include "models/Journal.h"

#include <cctype>

#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

namespace {
bool looksLikeIssn(const std::string& s) {  // NNNN-NNN[N|X]
    if (s.size() != 9 || s[4] != '-') return false;
    for (size_t i = 0; i < 9; ++i) {
        if (i == 4) continue;
        const unsigned char c = static_cast<unsigned char>(s[i]);
        if (i == 8 ? !(std::isdigit(c) || c == 'X' || c == 'x') : !std::isdigit(c)) return false;
    }
    return true;
}
}  // namespace

Journal::Journal(int id, std::string title, std::string author, std::string genre, int publicationYear,
                 int totalCopies, int volume, int issue, std::string issn, std::string academicField)
    : LibraryResource(id, std::move(title), std::move(author), "", std::move(genre), publicationYear,
                      totalCopies),
      volume_(volume),
      issue_(issue),
      issn_(StringUtils::toUpper(StringUtils::trim(issn))),
      academicField_(StringUtils::trim(academicField)) {
    requireIdentifier(issn_, "ISSN");
    if (!looksLikeIssn(issn_)) throw ValidationException("Invalid ISSN '" + issn_ + "' (expected format 1234-5678).");
    if (volume_ < 0 || issue_ < 0) throw ValidationException("Volume and issue cannot be negative.");
}

std::string Journal::getDescription() const {
    std::string text = "Academic journal, volume " + std::to_string(volume_) + ", issue " + std::to_string(issue_);
    if (!academicField_.empty()) text += ", field: " + academicField_;
    return text + ". Reference material; late fee $" + StringUtils::formatMoney(LATE_FEE_PER_DAY) + "/day.";
}

LibraryResource::Attributes Journal::getExtraAttributes() const {
    return {{"volume", std::to_string(volume_)},
            {"issue", std::to_string(issue_)},
            {"issn", issn_},
            {"academicField", academicField_}};
}
