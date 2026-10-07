#include "models/Library.h"

#include <algorithm>

#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

namespace {
template <typename List>
auto* findById(List& list, int id) {
    auto it = std::find_if(list.begin(), list.end(), [id](const auto& item) { return item->getId() == id; });
    return it == list.end() ? nullptr : it->get();
}
}  // namespace

std::vector<Reservation> Library::allReservations() const {
    std::vector<Reservation> all = processedReservations_;
    for (const auto& entry : waitingQueues_) {
        std::queue<Reservation> copy = entry.second;  // std::queue has no iterators: drain a copy
        while (!copy.empty()) { all.push_back(copy.front()); copy.pop(); }
    }
    std::sort(all.begin(), all.end(),
              [](const Reservation& a, const Reservation& b) { return a.getReservationId() < b.getReservationId(); });
    return all;
}

LibraryResource* Library::findResource(int id) { return findById(resources_, id); }
const LibraryResource* Library::findResource(int id) const { return findById(resources_, id); }

Member* Library::findMember(int id) {
    auto it = std::find_if(members_.begin(), members_.end(), [id](const auto& m) { return m->getMemberId() == id; });
    return it == members_.end() ? nullptr : it->get();
}
const Member* Library::findMember(int id) const {
    auto it = std::find_if(members_.begin(), members_.end(), [id](const auto& m) { return m->getMemberId() == id; });
    return it == members_.end() ? nullptr : it->get();
}

Loan* Library::findLoan(int id) {
    auto it = std::find_if(loans_.begin(), loans_.end(), [id](const Loan& l) { return l.getLoanId() == id; });
    return it == loans_.end() ? nullptr : &*it;
}
const Loan* Library::findLoan(int id) const {
    auto it = std::find_if(loans_.begin(), loans_.end(), [id](const Loan& l) { return l.getLoanId() == id; });
    return it == loans_.end() ? nullptr : &*it;
}

void Library::setName(const std::string& name) {
    const std::string trimmed = StringUtils::trim(name);
    if (trimmed.empty()) throw ValidationException("Library name cannot be empty.");
    if (trimmed.size() > 80) throw ValidationException("Library name is too long (max 80 characters).");
    name_ = trimmed;
}

void Library::clear() {
    resources_.clear();
    members_.clear();
    loans_.clear();
    waitingQueues_.clear();
    processedReservations_.clear();
    ids_ = IdCounters{};
    demoData_ = false;
}

void Library::adopt(Library&& other) {
    resources_ = std::move(other.resources_);
    members_ = std::move(other.members_);
    loans_ = std::move(other.loans_);
    waitingQueues_ = std::move(other.waitingQueues_);
    processedReservations_ = std::move(other.processedReservations_);
    ids_ = other.ids_;
    name_ = other.name_;
    demoData_ = other.demoData_;
}
