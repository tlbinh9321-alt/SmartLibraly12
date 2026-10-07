#pragma once
#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "models/Loan.h"
#include "models/LibraryResource.h"
#include "models/Member.h"
#include "models/Reservation.h"

// Aggregate root (COMPOSITION): the Library owns its resources, members, loans
// and reservations. It only stores and looks things up; business rules live in
// the services, which keeps this class small and each service testable.
class Library {
public:
    using ResourceList = std::vector<std::unique_ptr<LibraryResource>>;
    using MemberList = std::vector<std::unique_ptr<Member>>;
    // resourceId -> FIFO queue of WAITING reservations
    using WaitingQueues = std::map<int, std::queue<Reservation>>;

    struct IdCounters { int resource = 1, member = 1, loan = 1, reservation = 1; };

    ResourceList& resources() { return resources_; }
    const ResourceList& resources() const { return resources_; }
    MemberList& members() { return members_; }
    const MemberList& members() const { return members_; }
    std::vector<Loan>& loans() { return loans_; }
    const std::vector<Loan>& loans() const { return loans_; }
    WaitingQueues& waitingQueues() { return waitingQueues_; }
    const WaitingQueues& waitingQueues() const { return waitingQueues_; }
    // READY / COMPLETED / CANCELLED reservations (they have left the queue)
    std::vector<Reservation>& processedReservations() { return processedReservations_; }
    const std::vector<Reservation>& processedReservations() const { return processedReservations_; }
    std::vector<Reservation> allReservations() const;  // queue + processed, sorted by id

    LibraryResource* findResource(int id);
    const LibraryResource* findResource(int id) const;
    Member* findMember(int id);
    const Member* findMember(int id) const;
    Loan* findLoan(int id);
    const Loan* findLoan(int id) const;

    IdCounters& ids() { return ids_; }
    const IdCounters& ids() const { return ids_; }

    const std::string& getName() const { return name_; }
    void setName(const std::string& name);
    bool isDemoData() const { return demoData_; }
    void setDemoData(bool demo) { demoData_ = demo; }

    void clear();
    void adopt(Library&& other);  // replace all data, keep the change listener

    // Observer hook: services call notifyChanged() after every successful mutation;
    // the application wires it to PersistenceService::save().
    void setChangeListener(std::function<void()> listener) { listener_ = std::move(listener); }
    void muteNotifications(bool muted) { muted_ = muted; }
    void notifyChanged() const { if (listener_ && !muted_) listener_(); }

private:
    ResourceList resources_;
    MemberList members_;
    std::vector<Loan> loans_;
    WaitingQueues waitingQueues_;
    std::vector<Reservation> processedReservations_;
    IdCounters ids_;
    std::string name_ = "Smart Library";
    bool demoData_ = false;
    std::function<void()> listener_;
    bool muted_ = false;
};
