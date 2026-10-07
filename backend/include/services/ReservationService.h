#pragma once
#include <string>
#include <vector>

#include "models/Library.h"

// Owns the waiting-queue rules. Each resource has a std::queue<Reservation>
// (FIFO). When a copy comes back the first WAITING reservation becomes READY and
// that copy is held for that member only.
class ReservationService {
public:
    explicit ReservationService(Library& library) : library_(library) {}

    Reservation reserve(int memberId, int resourceId, const std::string& date = "");
    Reservation cancel(int reservationId);
    void complete(int reservationId);  // READY -> COMPLETED (called when the member borrows)

    // Promote WAITING -> READY while free copies exist. Does NOT notify: callers do.
    std::vector<Reservation> fulfillQueue(int resourceId);

    std::vector<Reservation> getQueue(int resourceId) const;  // WAITING, first in line first
    std::vector<Reservation> getAll() const;
    std::vector<Reservation> getForMember(int memberId) const;
    const Reservation* findReady(int memberId, int resourceId) const;
    int readyCount(int resourceId) const;
    int waitingCount() const;
    // Copies that a random member may borrow right now (free copies minus holds).
    int effectiveAvailableCopies(const LibraryResource& resource) const;
    bool hasOpenReservation(int memberId, int resourceId) const;

private:
    Library& library_;
};
