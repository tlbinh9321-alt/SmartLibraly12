#include "services/MemberService.h"

#include <algorithm>

#include "utils/Exceptions.h"
#include "utils/StringUtils.h"

void MemberService::ensureUniqueEmail(const Member& candidate) const {
    for (const auto& existing : library_.members())
        if (existing->getMemberId() != candidate.getMemberId() &&
            StringUtils::equalsIgnoreCase(existing->getEmail(), candidate.getEmail()))
            throw ConflictException("A member with this email already exists.");
}

Member& MemberService::create(const std::string& type, const FieldMap& fields) {
    auto member = MemberFactory::create(type, library_.ids().member, fields);
    ensureUniqueEmail(*member);
    ++library_.ids().member;
    library_.members().push_back(std::move(member));
    library_.notifyChanged();
    return *library_.members().back();
}

const Member& MemberService::getById(int id) const {
    const Member* member = library_.findMember(id);
    if (!member) throw NotFoundException("Member not found.");
    return *member;
}

std::vector<const Member*> MemberService::getAll() const {
    std::vector<const Member*> all;
    std::transform(library_.members().begin(), library_.members().end(), std::back_inserter(all),
                   [](const auto& m) { return m.get(); });
    return all;
}

Member& MemberService::update(int id, const FieldMap& changes) {
    Member* current = library_.findMember(id);
    if (!current) throw NotFoundException("Member not found.");
    FieldMap merged = MemberFactory::toFields(*current);
    for (const auto& change : changes) merged[change.first] = change.second;
    auto type = changes.find("type");
    auto replacement = MemberFactory::create(type == changes.end() ? current->getMemberType() : type->second, id, merged);
    ensureUniqueEmail(*replacement);
    for (auto& slot : library_.members())
        if (slot->getMemberId() == id) slot = std::move(replacement);
    library_.notifyChanged();
    return *library_.findMember(id);
}
