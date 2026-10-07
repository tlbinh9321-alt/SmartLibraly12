#pragma once
#include <stdexcept>
#include <string>

// Every business-rule failure is a LibraryException carrying the HTTP status
// the API layer should answer with. Domain code never knows about HTTP itself.
class LibraryException : public std::runtime_error {
public:
    LibraryException(const std::string& message, int httpStatus)
        : std::runtime_error(message), httpStatus_(httpStatus) {}
    int httpStatus() const { return httpStatus_; }

private:
    int httpStatus_;
};

class ValidationException : public LibraryException {
public:
    explicit ValidationException(const std::string& m) : LibraryException(m, 400) {}
};
class NotFoundException : public LibraryException {
public:
    explicit NotFoundException(const std::string& m) : LibraryException(m, 404) {}
};
class ConflictException : public LibraryException {
public:
    explicit ConflictException(const std::string& m) : LibraryException(m, 409) {}
};
class PersistenceException : public LibraryException {
public:
    explicit PersistenceException(const std::string& m) : LibraryException(m, 500) {}
};
