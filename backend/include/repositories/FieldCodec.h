#pragma once
#include <string>
#include <vector>

// Line format used by every data file: fields separated by '|'.
// Inside a field, '\' '|' and line breaks are escaped (\\ \| \n \r) so any text
// round-trips safely.
namespace FieldCodec {
std::string escape(const std::string& field);
std::string join(const std::vector<std::string>& fields);
std::vector<std::string> split(const std::string& line);
}  // namespace FieldCodec
