#ifndef _FLATEARTH_ENGINE_PLATFORM_HPP
#define _FLATEARTH_ENGINE_PLATFORM_HPP

#include "Core/FeMemory.hpp"
#include "Core/Input.hpp"
#include "Defines.hpp"
#include "Error.hpp"

#if FEPLATFORM_WINDOWS
#include <cstring>
#include <stdexcept>
#include <windows.h>
#include <windowsx.h>
#endif

namespace flatearth::platform {

struct InternalState;

struct PlatformState {
  FePtr<InternalState> internalState;
};

class Platform {
public:
  FEAPI explicit Platform(const string &applicationName, int32 x, int32 y, int32 width,
           int32 height, memory::MemoryManager &memManeger,
           input::InputManager &inputManager, event::EventManager &eventManager);
  ~Platform();

  FEAPI FeExpect<void, Error> Initialize();
  FEAPI FeExpect<bool, Error> PollEvents();
  FEAPI PlatformState *State();

#if FEPLATFORM_WINDOWS
  static LRESULT CALLBACK Win32ProcessMessage(HWND hwnd, uint32 msg, WPARAM wParam, LPARAM lParam);

private:
  LRESULT ProcessMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

private:
  PlatformState _platState;
  int32 _xPos, _yPos, _width, _height;
  string _applicationName;
  memory::MemoryManager &_memoryManager;
  input::InputManager &_inputManager;
  event::EventManager &_eventManager;
};

} // namespace flatearth::platform

#endif // _FLATEARTH_ENGINE_PLATFORM_HPP
