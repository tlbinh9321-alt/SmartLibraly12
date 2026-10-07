#pragma once
#include <filesystem>
#include <memory>
#include <string>

#include "models/Factories.h"
#include "services/LibraryApp.h"
#include "utils/Exceptions.h"

// A LibraryApp that stores its files in a fresh temporary directory.
struct TestApp {
    std::filesystem::path dir;
    std::unique_ptr<LibraryApp> app;
    explicit TestApp(const std::string& name);
    ~TestApp();
    LibraryApp* operator->() { return app.get(); }
    LibraryApp& operator*() { return *app; }
};

FieldMap bookFields(const std::string& title, const std::string& isbn, int copies = 1,
                    const std::string& author = "Some Author", const std::string& genre = "Programming");
FieldMap studentFields(const std::string& name, const std::string& email);
