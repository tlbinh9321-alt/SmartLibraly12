#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "models/LibraryResource.h"
#include "models/Member.h"

// Both the REST API and the file loader describe objects as string key/value
// maps; the factories turn them into the right concrete subclass.
using FieldMap = std::map<std::string, std::string>;

class ResourceFactory {
public:
    // type: BOOK | EBOOK | JOURNAL (case-insensitive). Throws ValidationException.
    static std::unique_ptr<LibraryResource> create(const std::string& type, int id, const FieldMap& fields);
    static std::vector<std::string> supportedTypes() { return {"BOOK", "EBOOK", "JOURNAL"}; }
    // Inverse of create(): the fields that describe an existing resource.
    static FieldMap toFields(const LibraryResource& resource);
};

class MemberFactory {
public:
    // type: STUDENT | FACULTY (case-insensitive). Throws ValidationException.
    static std::unique_ptr<Member> create(const std::string& type, int id, const FieldMap& fields);
    static FieldMap toFields(const Member& member);
};
