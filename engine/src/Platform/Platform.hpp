#ifndef _FLATEARTH_ENGINE_PLATFORM_HPP
#define _FLATEARTH_ENGINE_PLATFORM_HPP

#include "Core/FeMemory.hpp"
#include "Core/Input.hpp"
#include "Defines.hpp"
#include "Error.hpp"

namespace flatearth::platform {

struct InternalState;

struct PlatformState {
  FePtr<InternalState> internalState;
};

class Platform {
public:
  explicit Platform(const string &applicationName, int32 x, int32 y, int32 width,
           int32 height, memory::MemoryManager &memManeger,
           input::InputManager &inputManager);
  ~Platform();

  FeExpect<void, Error> Initialize();
  FeExpect<bool, Error> PollEvents();
  PlatformState *State();

private:
  PlatformState _platState;
  int32 _xPos, _yPos, _width, _height;
  string _applicationName;
  memory::MemoryManager &_memoryManager;
  input::InputManager &_inputManager;
};

} // namespace flatearth::platform

#endif // _FLATEARTH_ENGINE_PLATFORM_HPP
