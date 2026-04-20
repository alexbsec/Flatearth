#include <Core/Logger.hpp>

extern int RunAllTests();

int main() {
    febundle::core::Logger::Self().SetLevel(febundle::core::LogLevel::Off);
    return RunAllTests();
}
