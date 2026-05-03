#include "Core/Logger.hpp"
#include "Defines.hpp"

#include <cstdlib>

namespace flatearth::detail {

[[noreturn]] void FatalLog(stringv userMsg, stringv errMsg) {
  FLOG_FATAL("{}: {}", userMsg, errMsg);
  std::exit(1);
}

void ErrorLog(stringv userMsg, stringv errMsg, std::source_location loc) {
  febundle::core::Logger::Self().Log(
      febundle::core::LogLevel::Error, loc, "{}: {}", userMsg, errMsg);
}

void WarnLog(stringv userMsg, stringv errMsg, std::source_location loc) {
  if (errMsg.empty()) {
    febundle::core::Logger::Self().Log(febundle::core::LogLevel::Warn, loc, "{}", userMsg);
  } else {
    febundle::core::Logger::Self().Log(
        febundle::core::LogLevel::Warn, loc, "{}: {}", userMsg, errMsg);
  }
}

} // namespace flatearth::detail
