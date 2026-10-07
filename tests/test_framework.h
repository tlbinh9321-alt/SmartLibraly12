#pragma once
// Tiny self-contained test framework (no downloads needed).
#include <cmath>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

struct TestCase { std::string name; std::function<void()> body; };
std::vector<TestCase>& testRegistry();
struct TestRegistrar { TestRegistrar(const std::string& n, std::function<void()> f) { testRegistry().push_back({n, std::move(f)}); } };
struct TestFailure { std::string message; };

#define TEST(name)                                  \
    static void name();                             \
    static TestRegistrar registrar_##name(#name, name); \
    static void name()

#define FAIL_AT(text) throw TestFailure{std::string(__FILE__) + ":" + std::to_string(__LINE__) + "  " + (text)}
#define CHECK(cond) do { if (!(cond)) FAIL_AT(std::string("CHECK failed: ") + #cond); } while (0)
#define CHECK_EQ(a, b) do { auto va_ = (a); auto vb_ = (b); if (!(va_ == vb_)) { \
    std::ostringstream os_; os_ << "CHECK_EQ failed: " #a " == " #b "  (" << va_ << " vs " << vb_ << ")"; FAIL_AT(os_.str()); } } while (0)
#define CHECK_NEAR(a, b) do { double va_ = (a), vb_ = (b); if (std::fabs(va_ - vb_) > 0.0001) { \
    std::ostringstream os_; os_ << "CHECK_NEAR failed: " #a " ~ " #b "  (" << va_ << " vs " << vb_ << ")"; FAIL_AT(os_.str()); } } while (0)
// expr must throw ExType whose message contains `text`
#define CHECK_THROWS(expr, ExType, text) do { bool thrown_ = false; try { (void)(expr); } \
    catch (const ExType& e_) { thrown_ = true; if (std::string(e_.what()).find(text) == std::string::npos) \
        FAIL_AT(std::string("wrong message: '") + e_.what() + "' (expected to contain '" + text + "')"); } \
    if (!thrown_) FAIL_AT(std::string("expected ") + #ExType + " from: " #expr); } while (0)
