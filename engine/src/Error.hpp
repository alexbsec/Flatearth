#ifndef _FLATEARTH_ENGINE_ERROR_HPP
#define _FLATEARTH_ENGINE_ERROR_HPP

#include "Defines.hpp"
#include <source_location>

namespace flatearth {

struct Error {
  string message{"unknown error"};
  std::source_location where = std::source_location::current();

  inline Error() {}
  inline Error(const string &message) : message(message) {}
};

} // namespace flatearth

#endif // _FLATEARTH_ENGINE_ERROR_HPP
