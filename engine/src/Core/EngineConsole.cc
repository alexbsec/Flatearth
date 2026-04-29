#include "EngineConsole.hpp"

#include "Core/Input.hpp"
#include "Core/KVarRegistry.hpp"
#include "imgui.h"

namespace flatearth {

using namespace input;

DevConsole::DevConsole(input::InputManager &im) : _isOpen(FeFalse), _inputBuffer{}, _input(im) {
}

void DevConsole::Draw(KVarRegistry &kvars) {
  if (_input.IsKeyDown(KEY_GRAVE) && !_input.WasKeyDown(KEY_GRAVE)) {
    _isOpen = !_isOpen;
    if (_isOpen) _justOpened = FeTrue;
  }

  if (!_isOpen) {
    return;
  }

  ImGui::Begin("Dev Console");

  ImGui::BeginChild("scrolling", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));
  for (const auto &line : _history) {
    ImGui::TextUnformatted(line.c_str());
  }

  if (_scrollToBottom) {
    ImGui::SetScrollHereY(1.0f);
  }
  _scrollToBottom = FeFalse;

  ImGui::EndChild();

  ImGui::Separator();
  if (_justOpened) {
    ImGui::SetKeyboardFocusHere(0);
    _justOpened = FeFalse;
  }
  bool execute = ImGui::InputText(
      "##input", _inputBuffer, sizeof(_inputBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
  ImGui::SameLine();
  execute |= ImGui::Button("Run");

  if (execute && _inputBuffer[0] != '\0') {
    Execute(_inputBuffer, kvars);
    _inputBuffer[0] = '\0';
    ImGui::SetKeyboardFocusHere(-1);
    _scrollToBottom = FeTrue;
  }

  ImGui::End();
}

void DevConsole::Execute(stringv cmd, KVarRegistry &kvars) {
  _history.push_back("> " + string(cmd));

  auto nextWord = [](stringv s, std::size_t from) -> std::pair<stringv, std::size_t> {
    std::size_t start = s.find_first_not_of(' ', from);
    if (start == stringv::npos) {
      return {{}, stringv::npos};
    }
    std::size_t end = s.find(' ', start);
    return {s.substr(start, end - start), end};
  };

  auto [verb, afterVerb] = nextWord(cmd, 0);
  auto [name, afterName] = nextWord(cmd, afterVerb);

  stringv value =
      (afterName != stringv::npos) ? cmd.substr(cmd.find_first_not_of(' ', afterName)) : stringv{};

  ConsoleCmd consoleCmd{ConsoleCmd::Null};
  if (verb == "set") {
    consoleCmd = ConsoleCmd::Set;
  } else if (verb == "get") {
    consoleCmd = ConsoleCmd::Get;
  } else if (verb == "list") {
    consoleCmd = ConsoleCmd::List;
  } else if (!verb.empty()) {
    consoleCmd = ConsoleCmd::Unknown;
  }

  if (consoleCmd == ConsoleCmd::Unknown) {
    _history.push_back("  unknown command: " + string(verb));
    return;
  }

  return DispatchCommand(consoleCmd, name, value, kvars);
}

void DevConsole::DispatchCommand(ConsoleCmd cmd, stringv name, stringv value, KVarRegistry &kvars) {
  switch (cmd) {
    case ConsoleCmd::Set: {
      if (name.empty() || value.empty()) {
        _history.push_back("  usage: set <name> <value>");
        return;
      }
      auto res = kvars.ParseAndSet(name, value);
      if (res.errored()) {
        _history.push_back("  error: " + res.error().message);
      } else {
        _history.push_back("  " + string(name) + " = " + kvars.GetAsString(name));
      }
      break;
    }
    case ConsoleCmd::Get: {
      if (name.empty()) {
        _history.push_back("  usage: get <name>");
        return;
      }
      _history.push_back("  " + string(name) + " = " + kvars.GetAsString(name));
      break;
    }
    case ConsoleCmd::List:
      kvars.ForEach([&](const string &k, KVar &kvar) {
        _history.push_back("  " + k + " = " + kvars.GetAsString(k) + "  -- " + kvar.description);
      });
      break;
    default:
      return;
  }
}

} // namespace flatearth
