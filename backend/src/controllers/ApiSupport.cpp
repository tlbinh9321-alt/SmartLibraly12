#include "controllers/ApiSupport.h"

#include <cerrno>
#include <cstdlib>

#include "utils/StringUtils.h"

namespace Api {

HttpResponse ok(const std::string& message, Json data, int status) {
    Json body = Json::object();
    body.set("success", true).set("message", message).set("data", data.isNull() ? Json::object() : std::move(data));
    return {status, "application/json; charset=utf-8", body.dump()};
}

HttpResponse fail(const std::string& message, int status) {
    Json body = Json::object();
    body.set("success", false).set("message", message);
    return {status, "application/json; charset=utf-8", body.dump()};
}

namespace {
bool parseStrictInt(const std::string& text, int& out) {
    const std::string t = StringUtils::trim(text);
    if (t.empty()) return false;
    char* end = nullptr;
    errno = 0;
    const long v = std::strtol(t.c_str(), &end, 10);
    if (*end != '\0' || errno == ERANGE || v < 1 || v > 1000000000L) return false;
    out = static_cast<int>(v);
    return true;
}
}  // namespace

int parseId(const std::string& text, const std::string& label) {
    int id = 0;
    if (!parseStrictInt(text, id)) throw ValidationException("Invalid " + label + " ID.");
    return id;
}

Json parseBody(const HttpRequest& request) {
    if (StringUtils::trim(request.body).empty()) return Json::object();
    Json body = Json::parse(request.body);
    if (!body.isObject()) throw ValidationException("Request body must be a JSON object.");
    return body;
}

FieldMap toFieldMap(const Json& object) {
    FieldMap fields;
    for (const auto& entry : object.entries())
        if (entry.second.isScalar()) fields[entry.first] = entry.second.scalarToString();
    return fields;
}

std::optional<int> optionalInt(const Json& body, const std::string& key, const std::string& errorMessage) {
    const Json* value = body.find(key);
    if (!value || value->isNull() || (value->isString() && StringUtils::trim(value->asString()).empty())) return std::nullopt;
    int id = 0;
    if (!value->isScalar() || !parseStrictInt(value->scalarToString(), id)) throw ValidationException(errorMessage);
    return id;
}

int requiredInt(const Json& body, const std::string& key, const std::string& errorMessage) {
    auto value = optionalInt(body, key, errorMessage);
    if (!value) throw ValidationException(errorMessage);
    return *value;
}

}  // namespace Api
