#ifndef _FLATEARTH_ENGINE_CORE_ENGINE_CONSOLE_HPP
#define _FLATEARTH_ENGINE_CORE_ENGINE_CONSOLE_HPP

#include "Core/Input.hpp"
#include "Core/KVarRegistry.hpp"
#include "Defines.hpp"

namespace flatearth {

constexpr uint32 cMaxConsoleInputBuffer = 256;

enum class ConsoleCmd {
  Null,
  Unknown,
  Get,
  Set,
  List,
};

class DevConsole {
public:
  explicit DevConsole(input::InputManager &);
  void Draw(KVarRegistry &);
  bool IsOpen() const { return _isOpen; }

private:
  void Execute(stringv cmd, KVarRegistry &);
  void DispatchCommand(ConsoleCmd cmd, stringv name, stringv value, KVarRegistry &);

private:
  bool _isOpen;
  bool _justOpened{FeFalse};
  char _inputBuffer[cMaxConsoleInputBuffer];
  bool _scrollToBottom{FeTrue};
  std::vector<string> _history;

  input::InputManager &_input;
};

} // namespace flatearth

#endif // _FLATEARTH_ENGINE_CORE_ENGINE_CONSOLE_HPP
