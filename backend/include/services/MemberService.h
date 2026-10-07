#pragma once
#include <vector>

#include "models/Factories.h"
#include "models/Library.h"

class MemberService {
public:
    explicit MemberService(Library& library) : library_(library) {}

    Member& create(const std::string& type, const FieldMap& fields);
    const Member& getById(int id) const;
    std::vector<const Member*> getAll() const;
    Member& update(int id, const FieldMap& changes);  // "type" may upgrade/downgrade the subscription

private:
    void ensureUniqueEmail(const Member& candidate) const;
    Library& library_;
};
