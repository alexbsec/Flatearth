#ifndef _FLATEARTH_ENGINE_PLATFORM_HPP
#define _FLATEARTH_ENGINE_PLATFORM_HPP

#include "Defines.hpp"
#include "Error.hpp"
#include "Core/FeMemroy.hpp"

namespace flatearth::platform {

struct InternalState;

struct PlatformState {
  FePtr<InternalState> internalState;
};

class Platform {
public:
  Platform(const string &applicationName, int32 x, int32 y, int32 width,
           int32 height, memory::MemoryManager &memManeger);
  ~Platform();

  FeExpect<void, Error> Initialize();
  bool PollEvents();
  PlatformState *State();

private:
  PlatformState _platState;
  int32 _xPos, _yPos, _width, _height;
  string _applicationName;
  memory::MemoryManager &_memoryManager;
};

} // namespace flatearth::platform

#endif // _FLATEARTH_ENGINE_PLATFORM_HPP
