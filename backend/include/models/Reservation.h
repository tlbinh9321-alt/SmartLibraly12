#pragma once
#include <string>

#include "models/Enums.h"

class Reservation {
public:
    Reservation(int reservationId, int memberId, int resourceId, std::string reservationDate,
                ReservationStatus status = ReservationStatus::WAITING)
        : reservationId_(reservationId), memberId_(memberId), resourceId_(resourceId),
          reservationDate_(std::move(reservationDate)), status_(status) {}

    int getReservationId() const { return reservationId_; }
    int getMemberId() const { return memberId_; }
    int getResourceId() const { return resourceId_; }
    const std::string& getReservationDate() const { return reservationDate_; }
    ReservationStatus getStatus() const { return status_; }
    void setStatus(ReservationStatus status) { status_ = status; }

    bool isOpen() const { return status_ == ReservationStatus::WAITING || status_ == ReservationStatus::READY; }

private:
    int reservationId_;
    int memberId_;
    int resourceId_;
    std::string reservationDate_;
    ReservationStatus status_;
};
