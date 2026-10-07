#include "repositories/LibraryFileRepository.h"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <functional>

#include "models/Factories.h"
#include "repositories/FieldCodec.h"
#include "utils/DateUtils.h"
#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

namespace fs = std::filesystem;

namespace {
const char* const kFiles[] = {"resources.txt", "members.txt", "loans.txt", "reservations.txt", "settings.txt", nullptr};

void writeAtomically(const fs::path& target, const std::vector<std::string>& lines, const std::string& header) {
    const fs::path temp = target.string() + ".tmp";
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out) throw PersistenceException("Cannot write " + temp.string());
        out << "# " << header << "\n";
        for (const std::string& line : lines) out << line << "\n";
        out.flush();
        if (!out) throw PersistenceException("Failed while writing " + temp.string());
    }
    std::error_code ec;
    fs::rename(temp, target, ec);
    if (ec) {  // some platforms refuse to rename over an existing file
        fs::remove(target, ec);
        fs::rename(temp, target, ec);
        if (ec) throw PersistenceException("Cannot replace " + target.string() + ": " + ec.message());
    }
}

// Calls handler(fields) for each data line; wraps any error with file + line number.
void readLines(const fs::path& path, const std::function<void(const std::vector<std::string>&)>& handler) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw PersistenceException("Cannot open " + path.string());
    std::string line;
    int lineNumber = 0;
    while (std::getline(in, line)) {
        ++lineNumber;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        try {
            handler(FieldCodec::split(line));
        } catch (const std::exception& e) {
            throw PersistenceException(path.filename().string() + " line " + std::to_string(lineNumber) + ": " + e.what());
        }
    }
}

int toInt(const std::string& text) {
    char* end = nullptr;
    errno = 0;
    const long v = std::strtol(text.c_str(), &end, 10);
    if (text.empty() || *end != '\0' || errno == ERANGE) throw PersistenceException("not a number: '" + text + "'");
    return static_cast<int>(v);
}

double toDouble(const std::string& text) {
    char* end = nullptr;
    const double v = std::strtod(text.c_str(), &end);
    if (text.empty() || *end != '\0') throw PersistenceException("not a number: '" + text + "'");
    return v;
}

void need(const std::vector<std::string>& f, size_t count) {
    if (f.size() < count) throw PersistenceException("expected at least " + std::to_string(count) + " fields");
}
}  // namespace

const char* const* LibraryFileRepository::fileNames() { return kFiles; }

bool LibraryFileRepository::hasData() const {
    return fs::exists(dir_ / "resources.txt") && fs::exists(dir_ / "members.txt");
}

void LibraryFileRepository::save(const Library& library) const {
    std::error_code ec;
    fs::create_directories(dir_, ec);
    if (ec) throw PersistenceException("Cannot create data directory " + dir_.string() + ": " + ec.message());

    std::vector<std::string> lines;
    for (const auto& r : library.resources()) {
        std::vector<std::string> f{r->getType(), std::to_string(r->getId()), r->getTitle(), r->getAuthor(), r->getIsbn(),
                                   r->getGenre(), std::to_string(r->getPublicationYear()),
                                   std::to_string(r->getTotalCopies()), std::to_string(r->getAvailableCopies())};
        for (const auto& attribute : r->getExtraAttributes()) f.push_back(attribute.first + "=" + attribute.second);
        lines.push_back(FieldCodec::join(f));
    }
    writeAtomically(dir_ / "resources.txt", lines, "type|id|title|author|isbn|genre|year|total|available|key=value...");

    lines.clear();
    for (const auto& m : library.members())
        lines.push_back(FieldCodec::join({m->getMemberType(), std::to_string(m->getMemberId()), m->getFullName(), m->getEmail(),
                                          m->getPhone(), toString(m->getStatus()), m->getRegistrationDate()}));
    writeAtomically(dir_ / "members.txt", lines, "type|id|fullName|email|phone|status|registrationDate");

    lines.clear();
    for (const Loan& l : library.loans())
        lines.push_back(FieldCodec::join({std::to_string(l.getLoanId()), std::to_string(l.getMemberId()),
                                          std::to_string(l.getResourceId()), l.getBorrowDate(), l.getDueDate(), l.getReturnDate(),
                                          toString(l.getStatus()), StringUtils::formatMoney(l.getFineAmount()),
                                          l.isFinePaid() ? "1" : "0"}));
    writeAtomically(dir_ / "loans.txt", lines, "id|memberId|resourceId|borrowDate|dueDate|returnDate|status|fine|finePaid");

    lines.clear();
    for (const Reservation& r : library.allReservations())
        lines.push_back(FieldCodec::join({std::to_string(r.getReservationId()), std::to_string(r.getMemberId()),
                                          std::to_string(r.getResourceId()), r.getReservationDate(), toString(r.getStatus())}));
    writeAtomically(dir_ / "reservations.txt", lines, "id|memberId|resourceId|date|status (WAITING rows form the queues, in id order)");

    const Library::IdCounters& ids = library.ids();
    lines = {"libraryName=" + library.getName(), std::string("demoData=") + (library.isDemoData() ? "1" : "0"),
             "nextResourceId=" + std::to_string(ids.resource), "nextMemberId=" + std::to_string(ids.member),
             "nextLoanId=" + std::to_string(ids.loan), "nextReservationId=" + std::to_string(ids.reservation),
             "savedAt=" + DateUtils::nowTimestamp()};
    for (std::string& line : lines) {  // keep values with '\n' safe
        const auto eq = line.find('=');
        line = line.substr(0, eq + 1) + FieldCodec::escape(line.substr(eq + 1));
    }
    writeAtomically(dir_ / "settings.txt", lines, "key=value");
}

void LibraryFileRepository::load(Library& target) const {
    for (const char* const* name = kFiles; *name; ++name)
        if (!fs::exists(dir_ / *name)) throw PersistenceException(std::string("Missing data file: ") + (dir_ / *name).string());

    Library staging;  // all-or-nothing: only adopted when every file parsed
    readLines(dir_ / "resources.txt", [&](const std::vector<std::string>& f) {
        need(f, 9);
        FieldMap fields{{"title", f[2]}, {"author", f[3]}, {"isbn", f[4]}, {"genre", f[5]},
                        {"publicationYear", f[6]}, {"totalCopies", f[7]}};
        for (size_t i = 9; i < f.size(); ++i) {
            const auto eq = f[i].find('=');
            if (eq == std::string::npos) throw PersistenceException("malformed attribute '" + f[i] + "'");
            fields[f[i].substr(0, eq)] = f[i].substr(eq + 1);
        }
        auto resource = ResourceFactory::create(f[0], toInt(f[1]), fields);
        resource->restoreAvailableCopies(toInt(f[8]));
        staging.resources().push_back(std::move(resource));
    });
    readLines(dir_ / "members.txt", [&](const std::vector<std::string>& f) {
        need(f, 7);
        staging.members().push_back(MemberFactory::create(
            f[0], toInt(f[1]),
            {{"fullName", f[2]}, {"email", f[3]}, {"phone", f[4]}, {"status", f[5]}, {"registrationDate", f[6]}}));
    });
    readLines(dir_ / "loans.txt", [&](const std::vector<std::string>& f) {
        need(f, 9);
        staging.loans().emplace_back(toInt(f[0]), toInt(f[1]), toInt(f[2]), f[3], f[4], f[5], parseLoanStatus(f[6]),
                                     toDouble(f[7]), f[8] == "1");
    });
    readLines(dir_ / "reservations.txt", [&](const std::vector<std::string>& f) {
        need(f, 5);
        Reservation r(toInt(f[0]), toInt(f[1]), toInt(f[2]), f[3], parseReservationStatus(f[4]));
        if (r.getStatus() == ReservationStatus::WAITING) staging.waitingQueues()[r.getResourceId()].push(r);
        else staging.processedReservations().push_back(r);
    });
    readLines(dir_ / "settings.txt", [&](const std::vector<std::string>& f) {
        const auto& line = f[0];
        const auto eq = line.find('=');
        if (eq == std::string::npos) return;
        const std::string key = line.substr(0, eq), value = line.substr(eq + 1);
        if (key == "libraryName") staging.setName(value);
        else if (key == "demoData") staging.setDemoData(value == "1");
        else if (key == "nextResourceId") staging.ids().resource = toInt(value);
        else if (key == "nextMemberId") staging.ids().member = toInt(value);
        else if (key == "nextLoanId") staging.ids().loan = toInt(value);
        else if (key == "nextReservationId") staging.ids().reservation = toInt(value);
    });

    // Safety net: id counters must always be ahead of every stored id.
    for (const auto& r : staging.resources()) staging.ids().resource = std::max(staging.ids().resource, r->getId() + 1);
    for (const auto& m : staging.members()) staging.ids().member = std::max(staging.ids().member, m->getMemberId() + 1);
    for (const Loan& l : staging.loans()) staging.ids().loan = std::max(staging.ids().loan, l.getLoanId() + 1);
    for (const Reservation& r : staging.allReservations())
        staging.ids().reservation = std::max(staging.ids().reservation, r.getReservationId() + 1);

    target.adopt(std::move(staging));
}
