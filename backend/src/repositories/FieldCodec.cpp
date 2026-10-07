#include "repositories/FieldCodec.h"

namespace FieldCodec {

std::string escape(const std::string& field) {
    std::string out;
    for (char c : field) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '|': out += "\\|"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            default: out += c;
        }
    }
    return out;
}

std::string join(const std::vector<std::string>& fields) {
    std::string line;
    for (size_t i = 0; i < fields.size(); ++i) {
        if (i > 0) line += '|';
        line += escape(fields[i]);
    }
    return line;
}

std::vector<std::string> split(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    for (size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (c == '\\' && i + 1 < line.size()) {
            const char next = line[++i];
            current += (next == 'n') ? '\n' : (next == 'r') ? '\r' : next;
        } else if (c == '|') {
            fields.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    fields.push_back(current);
    return fields;
}

}  // namespace FieldCodec
