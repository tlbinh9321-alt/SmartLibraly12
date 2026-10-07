#include "utils/Json.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "utils/Exceptions.h"

Json& Json::set(const std::string& key, Json value) {
    if (type_ == Type::Null) type_ = Type::Object;
    for (auto& entry : members_)
        if (entry.first == key) { entry.second = std::move(value); return *this; }
    members_.emplace_back(key, std::move(value));
    return *this;
}

Json& Json::push(Json value) {
    if (type_ == Type::Null) type_ = Type::Array;
    items_.push_back(std::move(value));
    return *this;
}

const Json* Json::find(const std::string& key) const {
    if (type_ != Type::Object) return nullptr;
    for (const auto& entry : members_)
        if (entry.first == key) return &entry.second;
    return nullptr;
}

std::string Json::scalarToString() const {
    switch (type_) {
        case Type::String: return string_;
        case Type::Bool: return bool_ ? "true" : "false";
        case Type::Number: {
            char buffer[64];
            if (std::floor(number_) == number_ && std::fabs(number_) < 1e15)
                std::snprintf(buffer, sizeof buffer, "%lld", static_cast<long long>(number_));
            else
                std::snprintf(buffer, sizeof buffer, "%.15g", number_);
            return buffer;
        }
        default: return "";
    }
}

namespace {
void appendEscaped(std::string& out, const std::string& text) {
    out += '"';
    for (unsigned char c : text) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buffer[8];
                    std::snprintf(buffer, sizeof buffer, "\\u%04x", c);
                    out += buffer;
                } else {
                    out += static_cast<char>(c);  // UTF-8 bytes pass through untouched
                }
        }
    }
    out += '"';
}

void appendUtf8(std::string& out, unsigned cp) {
    if (cp < 0x80) out += static_cast<char>(cp);
    else if (cp < 0x800) { out += static_cast<char>(0xC0 | (cp >> 6)); out += static_cast<char>(0x80 | (cp & 0x3F)); }
    else if (cp < 0x10000) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

class Parser {
public:
    explicit Parser(const std::string& text) : s_(text) {}

    Json parseDocument() {
        skipSpace();
        Json value = parseValue(0);
        skipSpace();
        if (pos_ != s_.size()) fail("unexpected trailing characters");
        return value;
    }

private:
    [[noreturn]] void fail(const std::string& why) const {
        throw ValidationException("Invalid JSON body: " + why + " (position " + std::to_string(pos_) + ").");
    }
    void skipSpace() { while (pos_ < s_.size() && std::isspace(static_cast<unsigned char>(s_[pos_]))) ++pos_; }
    bool consume(char c) { if (pos_ < s_.size() && s_[pos_] == c) { ++pos_; return true; } return false; }
    void expect(char c) { if (!consume(c)) fail(std::string("expected '") + c + "'"); }

    Json parseValue(int depth) {
        if (depth > 64) fail("nesting too deep");
        if (pos_ >= s_.size()) fail("unexpected end of input");
        const char c = s_[pos_];
        if (c == '{') return parseObject(depth);
        if (c == '[') return parseArray(depth);
        if (c == '"') return Json(parseString());
        if (literal("true")) return Json(true);
        if (literal("false")) return Json(false);
        if (literal("null")) return Json();
        return parseNumber();
    }

    bool literal(const char* word) {
        const std::string w(word);
        if (s_.compare(pos_, w.size(), w) == 0) { pos_ += w.size(); return true; }
        return false;
    }

    Json parseNumber() {
        const size_t start = pos_;
        while (pos_ < s_.size() && std::string("+-0123456789.eE").find(s_[pos_]) != std::string::npos) ++pos_;
        if (start == pos_) fail("unexpected character");
        char* end = nullptr;
        const std::string token = s_.substr(start, pos_ - start);
        const double value = std::strtod(token.c_str(), &end);
        if (end == nullptr || *end != '\0') fail("malformed number");
        return Json(value);
    }

    unsigned parseHex4() {
        if (pos_ + 4 > s_.size()) fail("truncated unicode escape");
        const unsigned v = static_cast<unsigned>(std::strtoul(s_.substr(pos_, 4).c_str(), nullptr, 16));
        pos_ += 4;
        return v;
    }

    std::string parseString() {
        expect('"');
        std::string out;
        while (true) {
            if (pos_ >= s_.size()) fail("unterminated string");
            char c = s_[pos_++];
            if (c == '"') break;
            if (c != '\\') { out += c; continue; }
            if (pos_ >= s_.size()) fail("bad escape");
            c = s_[pos_++];
            switch (c) {
                case 'n': out += '\n'; break;
                case 't': out += '\t'; break;
                case 'r': out += '\r'; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case '/': case '\\': case '"': out += c; break;
                case 'u': {
                    unsigned cp = parseHex4();
                    if (cp >= 0xD800 && cp <= 0xDBFF && s_.compare(pos_, 2, "\\u") == 0) {
                        pos_ += 2;
                        const unsigned low = parseHex4();
                        cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                    }
                    appendUtf8(out, cp);
                    break;
                }
                default: fail("unknown escape sequence");
            }
        }
        return out;
    }

    Json parseArray(int depth) {
        expect('[');
        Json result = Json::array();
        skipSpace();
        if (consume(']')) return result;
        while (true) {
            skipSpace();
            result.push(parseValue(depth + 1));
            skipSpace();
            if (consume(',')) continue;
            expect(']');
            return result;
        }
    }

    Json parseObject(int depth) {
        expect('{');
        Json result = Json::object();
        skipSpace();
        if (consume('}')) return result;
        while (true) {
            skipSpace();
            std::string key = parseString();
            skipSpace();
            expect(':');
            skipSpace();
            result.set(key, parseValue(depth + 1));
            skipSpace();
            if (consume(',')) continue;
            expect('}');
            return result;
        }
    }

    const std::string& s_;
    size_t pos_ = 0;
};
}  // namespace

void Json::dumpTo(std::string& out) const {
    switch (type_) {
        case Type::Null: out += "null"; break;
        case Type::Bool: out += bool_ ? "true" : "false"; break;
        case Type::Number: out += scalarToString(); break;
        case Type::String: appendEscaped(out, string_); break;
        case Type::Array: {
            out += '[';
            bool first = true;
            for (const auto& item : items_) { if (!first) out += ','; first = false; item.dumpTo(out); }
            out += ']';
            break;
        }
        case Type::Object: {
            out += '{';
            bool first = true;
            for (const auto& entry : members_) {
                if (!first) out += ',';
                first = false;
                appendEscaped(out, entry.first);
                out += ':';
                entry.second.dumpTo(out);
            }
            out += '}';
            break;
        }
    }
}

std::string Json::dump() const { std::string out; dumpTo(out); return out; }

Json Json::parse(const std::string& text) { return Parser(text).parseDocument(); }
