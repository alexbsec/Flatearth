#include "Defines.hpp"
#include "Core/Logger.hpp"

#include <cstdlib>

namespace flatearth::detail {

[[noreturn]] void FatalLog(stringv userMsg, stringv errMsg) {
  FLOG_FATAL("{}: {}", userMsg, errMsg);
  std::exit(1);
}

void ErrorLog(stringv userMsg, stringv errMsg) {
  FLOG_ERROR("{}: {}", userMsg, errMsg);
}

void WarnLog(stringv userMsg, stringv errMsg) {
  if (errMsg.empty()) {
    FLOG_WARN("{}", userMsg);
  } else {
    FLOG_WARN("{}: {}", userMsg, errMsg);
  }
}

} // namespace flatearth::detail
