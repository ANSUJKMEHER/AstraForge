#pragma once
// af_test — minimal header-only test framework for AstraForge.
//
// Why not Catch2/doctest/GTest? The project's dependency policy is to keep the
// external surface to a single library (SDL2). A ~100-line framework is
// sufficient for deterministic unit tests, is trivial to explain in an
// interview, and adds zero build complexity on Windows/MSVC (no FetchContent,
// no vcpkg ports). Trade-off: no fixtures/parameterization — not needed at
// this project's scale. See context.md → "Decisions".
//
// Usage:
//   AF_TEST("name") {
//       AF_CHECK(expr);
//       AF_CHECK_EQ(a, b);
//       AF_CHECK_NEAR(a, b, eps);
//   }
// One translation unit calls RunAll() from main().

#include <cstdio>
#include <vector>

#include "af/math/MathUtil.h"

namespace af::test {

struct Case {
    const char* name;
    void (*fn)();
};

inline std::vector<Case>& Registry() {
    static std::vector<Case> registry;
    return registry;
}

struct Registrar {
    Registrar(const char* name, void (*fn)()) { Registry().push_back({name, fn}); }
};

struct State {
    int checks = 0;
    int failures = 0;
};

inline State& CurrentState() {
    static State state;
    return state;
}

inline void ReportFailure(const char* file, int line, const char* what) {
    ++CurrentState().failures;
    std::printf("  FAIL %s:%d  %s\n", file, line, what);
}

inline int RunAll() {
    int failedCases = 0;
    for (const Case& c : Registry()) {
        const int checksBefore = CurrentState().checks;
        const int failuresBefore = CurrentState().failures;
        c.fn();
        const int caseChecks = CurrentState().checks - checksBefore;
        const int caseFailures = CurrentState().failures - failuresBefore;
        if (caseFailures == 0) {
            std::printf("[PASS] %-48s (%d checks)\n", c.name, caseChecks);
        } else {
            ++failedCases;
            std::printf("[FAIL] %-48s (%d of %d checks failed)\n", c.name, caseFailures, caseChecks);
        }
    }
    std::printf("\n%d test case(s), %d check(s), %d failure(s)\n",
                static_cast<int>(Registry().size()), CurrentState().checks,
                CurrentState().failures);
    return failedCases == 0 ? 0 : 1;
}

}  // namespace af::test

#define AF_TEST_CONCAT_INNER(a, b) a##b
#define AF_TEST_CONCAT(a, b) AF_TEST_CONCAT_INNER(a, b)

#define AF_TEST(name)                                                          \
    static void AF_TEST_CONCAT(afTestFn, __LINE__)();                          \
    static ::af::test::Registrar AF_TEST_CONCAT(afTestReg, __LINE__)(          \
        name, &AF_TEST_CONCAT(afTestFn, __LINE__));                            \
    static void AF_TEST_CONCAT(afTestFn, __LINE__)()

#define AF_CHECK(expr)                                                         \
    do {                                                                       \
        ++::af::test::CurrentState().checks;                                   \
        if (!(expr)) {                                                         \
            ::af::test::ReportFailure(__FILE__, __LINE__, #expr);              \
        }                                                                      \
    } while (0)

#define AF_CHECK_EQ(a, b)                                                      \
    do {                                                                       \
        ++::af::test::CurrentState().checks;                                   \
        if (!((a) == (b))) {                                                   \
            ::af::test::ReportFailure(__FILE__, __LINE__, #a " == " #b);       \
        }                                                                      \
    } while (0)

#define AF_CHECK_NEAR(a, b, eps)                                               \
    do {                                                                       \
        ++::af::test::CurrentState().checks;                                   \
        const auto afVa = (a);                                                 \
        const auto afVb = (b);                                                 \
        const auto afEps = (eps);                                              \
        if (!(::af::Abs(afVa - afVb) <= afEps)) {                              \
            char afBuf[176];                                                   \
            std::snprintf(afBuf, sizeof(afBuf),                                \
                          "%s ~= %s  (a=%.6f, b=%.6f, eps=%.6f)",               \
                          #a, #b, static_cast<double>(afVa),                   \
                          static_cast<double>(afVb), static_cast<double>(afEps)); \
            ::af::test::ReportFailure(__FILE__, __LINE__, afBuf);              \
        }                                                                      \
    } while (0)
