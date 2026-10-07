#include "services/ReservationService.h"

#include <algorithm>

#include "utils/DateUtils.h"
#include "utils/Exceptions.h"

namespace {
std::vector<Reservation> drain(std::queue<Reservation> queue) {  // by value: iterate a copy
    std::vector<Reservation> out;
    while (!queue.empty()) { out.push_back(queue.front()); queue.pop(); }
    return out;
}
}  // namespace

int ReservationService::readyCount(int resourceId) const {
    const auto& processed = library_.processedReservations();
    return static_cast<int>(std::count_if(processed.begin(), processed.end(), [resourceId](const Reservation& r) {
        return r.getResourceId() == resourceId && r.getStatus() == ReservationStatus::READY;
    }));
}

int ReservationService::effectiveAvailableCopies(const LibraryResource& resource) const {
    return std::max(0, resource.getAvailableCopies() - readyCount(resource.getId()));
}

const Reservation* ReservationService::findReady(int memberId, int resourceId) const {
    const auto& processed = library_.processedReservations();
    auto it = std::find_if(processed.begin(), processed.end(), [&](const Reservation& r) {
        return r.getMemberId() == memberId && r.getResourceId() == resourceId && r.getStatus() == ReservationStatus::READY;
    });
    return it == processed.end() ? nullptr : &*it;
}

bool ReservationService::hasOpenReservation(int memberId, int resourceId) const {
    if (findReady(memberId, resourceId)) return true;
    auto it = library_.waitingQueues().find(resourceId);
    if (it == library_.waitingQueues().end()) return false;
    const auto waiting = drain(it->second);
    return std::any_of(waiting.begin(), waiting.end(), [memberId](const Reservation& r) { return r.getMemberId() == memberId; });
}

Reservation ReservationService::reserve(int memberId, int resourceId, const std::string& date) {
    const Member* member = library_.findMember(memberId);
    if (!member) throw NotFoundException("Member not found.");
    member->ensureCanBorrow();
    const LibraryResource* resource = library_.findResource(resourceId);
    if (!resource) throw NotFoundException("Resource not found.");

    for (const Loan& loan : library_.loans())
        if (loan.isActive() && loan.getMemberId() == memberId && loan.getResourceId() == resourceId)
            throw ConflictException("Member already has this resource on loan.");
    if (hasOpenReservation(memberId, resourceId))
        throw ConflictException("Duplicate reservation: this member is already queued for this resource.");
    if (effectiveAvailableCopies(*resource) > 0)
        throw ValidationException("Resource is available - borrow it instead of reserving it.");

    const std::string reservationDate = date.empty() ? DateUtils::today() : date;
    if (!DateUtils::isValid(reservationDate)) throw ValidationException("Invalid reservation date (expected YYYY-MM-DD).");
    Reservation reservation(library_.ids().reservation, memberId, resourceId, reservationDate);
    ++library_.ids().reservation;
    library_.waitingQueues()[resourceId].push(reservation);
    library_.notifyChanged();
    return reservation;
}

Reservation ReservationService::cancel(int reservationId) {
    auto& processed = library_.processedReservations();
    auto ready = std::find_if(processed.begin(), processed.end(), [reservationId](const Reservation& r) {
        return r.getReservationId() == reservationId && r.getStatus() == ReservationStatus::READY;
    });
    if (ready != processed.end()) {  // a held copy is released to the next member
        ready->setStatus(ReservationStatus::CANCELLED);
        Reservation cancelled = *ready;
        fulfillQueue(cancelled.getResourceId());
        library_.notifyChanged();
        return cancelled;
    }
    for (auto& entry : library_.waitingQueues()) {
        std::vector<Reservation> waiting = drain(entry.second);
        auto target = std::find_if(waiting.begin(), waiting.end(),
                                   [reservationId](const Reservation& r) { return r.getReservationId() == reservationId; });
        if (target == waiting.end()) continue;
        target->setStatus(ReservationStatus::CANCELLED);
        Reservation cancelled = *target;
        processed.push_back(cancelled);
        waiting.erase(target);
        std::queue<Reservation> rebuilt;  // rebuild the queue without the cancelled entry
        for (const Reservation& r : waiting) rebuilt.push(r);
        entry.second = rebuilt;
        if (entry.second.empty()) library_.waitingQueues().erase(entry.first);
        library_.notifyChanged();
        return cancelled;
    }
    auto done = std::find_if(processed.begin(), processed.end(),
                             [reservationId](const Reservation& r) { return r.getReservationId() == reservationId; });
    if (done != processed.end()) throw ConflictException("Reservation is already completed or cancelled.");
    throw NotFoundException("Reservation not found.");
}

void ReservationService::complete(int reservationId) {
    for (Reservation& r : library_.processedReservations())
        if (r.getReservationId() == reservationId && r.getStatus() == ReservationStatus::READY)
            r.setStatus(ReservationStatus::COMPLETED);
}

std::vector<Reservation> ReservationService::fulfillQueue(int resourceId) {
    std::vector<Reservation> promoted;
    const LibraryResource* resource = library_.findResource(resourceId);
    auto it = library_.waitingQueues().find(resourceId);
    if (!resource || it == library_.waitingQueues().end()) return promoted;
    std::queue<Reservation>& queue = it->second;
    while (!queue.empty() && effectiveAvailableCopies(*resource) > 0) {
        Reservation next = queue.front();  // first in line gets priority
        queue.pop();
        next.setStatus(ReservationStatus::READY);
        library_.processedReservations().push_back(next);
        promoted.push_back(next);
    }
    if (queue.empty()) library_.waitingQueues().erase(it);
    return promoted;
}

std::vector<Reservation> ReservationService::getQueue(int resourceId) const {
    auto it = library_.waitingQueues().find(resourceId);
    return it == library_.waitingQueues().end() ? std::vector<Reservation>{} : drain(it->second);
}

std::vector<Reservation> ReservationService::getAll() const { return library_.allReservations(); }

std::vector<Reservation> ReservationService::getForMember(int memberId) const {
    std::vector<Reservation> all = library_.allReservations(), mine;
    std::copy_if(all.begin(), all.end(), std::back_inserter(mine),
                 [memberId](const Reservation& r) { return r.getMemberId() == memberId; });
    return mine;
}

int ReservationService::waitingCount() const {
    int total = 0;
    for (const auto& entry : library_.waitingQueues()) total += static_cast<int>(entry.second.size());
    return total;
}
