#pragma once
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// Small JSON value type (writer + parser). Objects keep insertion order so
// API output is stable and easy to read.
class Json {
public:
    enum class Type { Null, Bool, Number, String, Array, Object };
    using Entry = std::pair<std::string, Json>;

    Json() = default;
    Json(bool value) : type_(Type::Bool), bool_(value) {}
    template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> && !std::is_same_v<T, bool>>>
    Json(T value) : type_(Type::Number), number_(static_cast<double>(value)) {}
    Json(const char* value) : type_(Type::String), string_(value) {}
    Json(std::string value) : type_(Type::String), string_(std::move(value)) {}

    static Json array() { Json j; j.type_ = Type::Array; return j; }
    static Json object() { Json j; j.type_ = Type::Object; return j; }

    Json& set(const std::string& key, Json value);  // objects
    Json& push(Json value);                         // arrays

    Type type() const { return type_; }
    bool isNull() const { return type_ == Type::Null; }
    bool isObject() const { return type_ == Type::Object; }
    bool isArray() const { return type_ == Type::Array; }
    bool isString() const { return type_ == Type::String; }
    bool isNumber() const { return type_ == Type::Number; }
    bool isBool() const { return type_ == Type::Bool; }
    bool isScalar() const { return isString() || isNumber() || isBool(); }

    const Json* find(const std::string& key) const;
    std::string scalarToString() const;  // "12", "true", "text" (empty for null/containers)
    bool asBool(bool fallback = false) const { return isBool() ? bool_ : fallback; }
    double asNumber(double fallback = 0) const { return isNumber() ? number_ : fallback; }
    const std::string& asString() const { return string_; }
    const std::vector<Json>& items() const { return items_; }
    const std::vector<Entry>& entries() const { return members_; }

    std::string dump() const;
    static Json parse(const std::string& text);  // throws ValidationException

private:
    void dumpTo(std::string& out) const;

    Type type_ = Type::Null;
    bool bool_ = false;
    double number_ = 0;
    std::string string_;
    std::vector<Json> items_;
    std::vector<Entry> members_;
};
