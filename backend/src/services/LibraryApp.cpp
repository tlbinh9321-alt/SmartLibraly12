#include "services/LibraryApp.h"

LibraryApp::LibraryApp(const std::string& dataDirectory, const std::string& reportsDirectory)
    : fines(),
      reservations(library),
      resources(library, reservations),
      members(library),
      loans(library, fines, reservations),
      search(library, reservations),
      reports(library, loans, reportsDirectory),
      persistence(library, dataDirectory) {}
