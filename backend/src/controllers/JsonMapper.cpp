#include "controllers/JsonMapper.h"

#include <algorithm>

#include "utils/DateUtils.h"
#include "utils/StringUtils.h"

Json JsonMapper::resource(const LibraryResource& r) const {
    const int effective = app_.reservations.effectiveAvailableCopies(r);
    Json extra = Json::object();
    for (const auto& attribute : r.getExtraAttributes()) extra.set(attribute.first, attribute.second);
    Json j = Json::object();
    j.set("id", r.getId()).set("title", r.getTitle()).set("author", r.getAuthor())
        .set("isbn", r.getIdentifier()).set("identifierLabel", r.getIdentifierLabel())
        .set("genre", r.getGenre()).set("publicationYear", r.getPublicationYear())
        .set("type", r.getType()).set("description", r.getDescription()).set("lateFeeRate", r.getLateFeeRate())
        .set("totalCopies", r.getTotalCopies()).set("availableCopies", r.getAvailableCopies())
        .set("borrowedCopies", r.getBorrowedCopies())
        .set("heldCopies", r.getAvailableCopies() - effective)  // free copies held for READY reservations
        .set("available", effective > 0).set("availability", effective > 0 ? "AVAILABLE" : "UNAVAILABLE")
        .set("queueLength", static_cast<int>(app_.reservations.getQueue(r.getId()).size()))
        .set("extra", std::move(extra));
    return j;
}

Json JsonMapper::member(const Member& m) const {
    const std::string today = DateUtils::today();
    Json j = Json::object();
    j.set("id", m.getMemberId()).set("fullName", m.getFullName()).set("email", m.getEmail()).set("phone", m.getPhone())
        .set("status", toString(m.getStatus())).set("registrationDate", m.getRegistrationDate())
        .set("memberType", m.getMemberType()).set("borrowLimit", m.getBorrowLimit())
        .set("loanDurationDays", m.getLoanDurationDays()).set("fineRate", m.getFineRate())
        .set("activeLoans", app_.loans.countActiveLoans(m.getMemberId()))
        .set("outstandingFines", StringUtils::round2(app_.loans.outstandingFinesForMember(m.getMemberId(), today)));
    return j;
}

Json JsonMapper::loan(const Loan& l) const {
    const std::string today = DateUtils::today();
    const Member* member = app_.library.findMember(l.getMemberId());
    const LibraryResource* resource = app_.library.findResource(l.getResourceId());
    const int overdueDays = app_.loans.calculateOverdueDays(l, today);
    std::string status = toString(l.getStatus());
    if (l.getStatus() == LoanStatus::BORROWED && overdueDays > 0) status = "OVERDUE";
    Json j = Json::object();
    j.set("id", l.getLoanId()).set("memberId", l.getMemberId())
        .set("memberName", member ? member->getFullName() : "(unknown member)")
        .set("resourceId", l.getResourceId())
        .set("resourceTitle", resource ? resource->getTitle() : "(deleted resource)")
        .set("resourceType", resource ? resource->getType() : "")
        .set("borrowDate", l.getBorrowDate()).set("dueDate", l.getDueDate()).set("returnDate", l.getReturnDate())
        .set("status", status).set("overdueDays", overdueDays)
        .set("fineAmount", l.getFineAmount()).set("finePaid", l.isFinePaid())
        .set("outstandingFine", StringUtils::round2(app_.loans.outstandingFine(l, today)));
    return j;
}

Json JsonMapper::loanList(const std::vector<Loan>& loans, size_t limit) const {
    std::vector<const Loan*> ordered;
    for (const Loan& l : loans) ordered.push_back(&l);
    std::sort(ordered.begin(), ordered.end(), [](const Loan* a, const Loan* b) { return a->getLoanId() > b->getLoanId(); });
    Json array = Json::array();
    for (const Loan* l : ordered) {
        if (limit != 0 && array.items().size() >= limit) break;
        array.push(loan(*l));
    }
    return array;
}

Json JsonMapper::reservation(const Reservation& r) const {
    const Member* member = app_.library.findMember(r.getMemberId());
    const LibraryResource* resource = app_.library.findResource(r.getResourceId());
    int position = 0;
    if (r.getStatus() == ReservationStatus::WAITING) {
        const auto queue = app_.reservations.getQueue(r.getResourceId());
        for (size_t i = 0; i < queue.size(); ++i)
            if (queue[i].getReservationId() == r.getReservationId()) position = static_cast<int>(i) + 1;
    }
    Json j = Json::object();
    j.set("id", r.getReservationId()).set("memberId", r.getMemberId())
        .set("memberName", member ? member->getFullName() : "(unknown member)")
        .set("resourceId", r.getResourceId())
        .set("resourceTitle", resource ? resource->getTitle() : "(deleted resource)")
        .set("reservationDate", r.getReservationDate()).set("status", toString(r.getStatus())).set("position", position);
    return j;
}

Json JsonMapper::queueOf(int resourceId) const {
    Json array = Json::array();
    for (const Reservation& r : app_.reservations.getQueue(resourceId)) array.push(reservation(r));
    return array;
}

Json JsonMapper::reservationQueues() const {
    Json result = Json::array();
    for (const auto& r : app_.library.resources()) {
        Json waiting = queueOf(r->getId());
        Json ready = Json::array();
        for (const Reservation& res : app_.library.processedReservations())
            if (res.getResourceId() == r->getId() && res.getStatus() == ReservationStatus::READY) ready.push(reservation(res));
        const bool unavailable = app_.reservations.effectiveAvailableCopies(*r) <= 0;
        if (waiting.items().empty() && ready.items().empty() && !unavailable) continue;
        Json entry = Json::object();
        entry.set("resource", resource(*r)).set("queue", std::move(waiting)).set("ready", ready)
            .set("nextMemberReady", !ready.items().empty());
        result.push(std::move(entry));
    }
    return result;
}

Json JsonMapper::stats(const InventoryStats& s) const {
    Json trend = Json::array();
    for (const auto& day : s.borrowingTrend) trend.push(Json::object().set("date", day.first).set("count", day.second));
    Json j = Json::object();
    j.set("generatedOn", s.generatedOn)
        .set("resources", Json::object().set("total", s.totalResources).set("books", s.books).set("ebooks", s.ebooks).set("journals", s.journals))
        .set("copies", Json::object().set("total", s.totalCopies).set("available", s.availableCopies).set("borrowed", s.borrowedCopies))
        .set("members", Json::object().set("total", s.totalMembers).set("students", s.students).set("faculty", s.faculty)
                            .set("active", s.activeMembers).set("inactive", s.inactiveMembers).set("suspended", s.suspendedMembers))
        .set("loans", Json::object().set("total", s.totalLoans).set("active", s.activeLoans).set("overdue", s.overdueLoans).set("returned", s.returnedLoans))
        .set("reservations", Json::object().set("total", s.totalReservations).set("waiting", s.waitingReservations).set("ready", s.readyReservations))
        .set("fines", Json::object().set("outstanding", s.outstandingFines).set("collected", s.collectedFines))
        .set("overdueBuckets", Json::array().push(s.overdueBuckets[0]).push(s.overdueBuckets[1]).push(s.overdueBuckets[2]))
        .set("borrowingTrend", std::move(trend));
    return j;
}
