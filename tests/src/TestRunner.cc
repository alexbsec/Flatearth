#include <chrono>
#include <cstdio>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#define CLR_GREEN "\x1b[32m"
#define CLR_RED   "\x1b[31m"
#define CLR_GREY  "\x1b[90m"
#define CLR_BOLD  "\x1b[1m"
#define CLR_RESET "\x1b[0m"

struct TestCase {
    std::string name;
    std::function<bool()> fn;
};

static std::vector<TestCase>& Registry() {
    static std::vector<TestCase> reg;
    return reg;
}

void RegisterTest(std::string_view name, std::function<bool()> fn) {
    Registry().push_back({std::string(name), fn});
}

// Returns the suite prefix: everything before the first ": ", or the full name.
static std::string_view SuiteOf(std::string_view name) {
    auto pos = name.find(": ");
    return pos == std::string_view::npos ? name : name.substr(0, pos);
}

int RunAllTests() {
    using Clock = std::chrono::steady_clock;
    auto& tests = Registry();

    int globalPassed = 0, globalFailed = 0;

    std::size_t i = 0;
    while (i < tests.size()) {
        std::string_view suite = SuiteOf(tests[i].name);

        // Collect all tests belonging to this suite
        std::size_t start = i;
        while (i < tests.size() && SuiteOf(tests[i].name) == suite) {
            ++i;
        }

        int suitePassed = 0, suiteFailed = 0;
        std::vector<std::string_view> failures;

        auto t0 = Clock::now();
        for (std::size_t j = start; j < i; ++j) {
            bool ok = tests[j].fn();
            ok ? ++suitePassed : ++suiteFailed;
            if (!ok) failures.push_back(tests[j].name);
        }
        double ms = std::chrono::duration<double, std::milli>(Clock::now() - t0).count();

        globalPassed += suitePassed;
        globalFailed += suiteFailed;

        if (suiteFailed == 0) {
            std::printf("%sok%s  %-32.*s %s[%d/%d]%s  %s%.2fms%s\n",
                        CLR_GREEN, CLR_RESET,
                        static_cast<int>(suite.size()), suite.data(),
                        CLR_GREY, suitePassed, suitePassed, CLR_RESET,
                        CLR_GREY, ms, CLR_RESET);
        } else {
            std::printf("%sFAIL%s %-32.*s %s[%d/%d]%s  %s%.2fms%s\n",
                        CLR_RED, CLR_RESET,
                        static_cast<int>(suite.size()), suite.data(),
                        CLR_GREY, suitePassed, suitePassed + suiteFailed, CLR_RESET,
                        CLR_GREY, ms, CLR_RESET);
            for (auto& fname : failures)
                std::printf("  %s--- FAIL: %.*s%s\n",
                            CLR_RED,
                            static_cast<int>(fname.size()), fname.data(),
                            CLR_RESET);
        }
    }

    int total = globalPassed + globalFailed;
    std::printf("\n%s%s%d/%d passed%s",
                CLR_BOLD, globalFailed == 0 ? CLR_GREEN : CLR_RED,
                globalPassed, total, CLR_RESET);
    if (globalFailed > 0) {
        std::printf("  %s%d failed%s", CLR_RED, globalFailed, CLR_RESET);
    }
    std::printf("\n");

    return globalFailed == 0 ? 0 : 1;
}
