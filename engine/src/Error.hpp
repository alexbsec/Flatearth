#ifndef _FLATEARTH_ENGINE_ERROR_HPP
#define _FLATEARTH_ENGINE_ERROR_HPP

#include "Defines.hpp"
#include <source_location>

namespace flatearth {

enum class ErrorType { Unknown,
  EventNotFound,
  EventAlreadyRegistered,
  GameResizeError,
  GameInitializeError,
  NoBackendRenderer,
  GameInitializeUndefined,
  RendererVulkanError,
  RendererSelectPhysicalDevice,
  GameUpdateUndefined,
  PlatformCreateSurface,
  GameResizeUndefined,
  NullptrException,
};

struct Error {
  ErrorType type{ErrorType::Unknown};
  string message{"unknown error"};
  std::source_location where = std::source_location::current();

  inline Error() {}
  inline Error(const string &message) : message(message) {}
  inline Error(const string &message, ErrorType type)
      : type(type), message(message) {}
};

} // namespace flatearth

#endif // _FLATEARTH_ENGINE_ERROR_HPP
