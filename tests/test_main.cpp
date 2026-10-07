#include <iostream>

#include "test_framework.h"
#include "test_helpers.h"

std::vector<TestCase>& testRegistry() { static std::vector<TestCase> tests; return tests; }

TestApp::TestApp(const std::string& name) {
    dir = std::filesystem::temp_directory_path() / ("smartlibrary_test_" + name);
    std::filesystem::remove_all(dir);
    app = std::make_unique<LibraryApp>((dir / "data").string(), (dir / "reports").string());
}
TestApp::~TestApp() { app.reset(); std::error_code ec; std::filesystem::remove_all(dir, ec); }

FieldMap bookFields(const std::string& title, const std::string& isbn, int copies, const std::string& author, const std::string& genre) {
    return {{"title", title}, {"author", author}, {"isbn", isbn}, {"genre", genre}, {"publicationYear", "2020"},
            {"totalCopies", std::to_string(copies)}, {"publisher", "Test Press"}, {"pageCount", "300"}, {"edition", "1st"}};
}
FieldMap studentFields(const std::string& name, const std::string& email) {
    return {{"fullName", name}, {"email", email}, {"phone", "+84 90 123 4567"}};
}

int main() {
    int passed = 0, failed = 0;
    for (const TestCase& test : testRegistry()) {
        try {
            test.body();
            std::cout << "[ PASS ] " << test.name << "\n";
            ++passed;
        } catch (const TestFailure& f) {
            std::cout << "[ FAIL ] " << test.name << "\n         " << f.message << "\n";
            ++failed;
        } catch (const std::exception& e) {
            std::cout << "[ FAIL ] " << test.name << "\n         unexpected exception: " << e.what() << "\n";
            ++failed;
        }
    }
    std::cout << "\n" << passed << " passed, " << failed << " failed, " << (passed + failed) << " total\n";
    return failed == 0 ? 0 : 1;
}
