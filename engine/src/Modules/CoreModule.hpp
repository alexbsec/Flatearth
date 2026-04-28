#ifndef _FLATEARTH_ENGINE_MODULES_CORE_MODULE_HPP
#define _FLATEARTH_ENGINE_MODULES_CORE_MODULE_HPP

#include "Audio/AudioManager.hpp"
#include "Core/FeMemory.hpp"
#include "Core/Input.hpp"
#include "Core/KVarRegistry.hpp"
#include "Defines.hpp"

namespace flatearth::modules {

class Core {
public:
  explicit Core(memory::MemoryManager &mm, event::EventManager &em)
      : _memoryManager(mm), _input(em), _audio(mm), _kvars(mm) {
  }

  FeExpect<bool, Error> Initialize();
  void Shutdown();

  // Gives access to Core Memory internals
  FEAPI inline memory::MemoryManager &Memory() { return _memoryManager; }
  FEAPI const memory::MemoryManager &Memory() const { return _memoryManager; }

  // Gives access to Input internals
  FEAPI input::InputManager &Input() { return _input; }
  FEAPI const input::InputManager &Input() const { return _input; }

  // Gives access to Audio internals
  FEAPI audio::AudioManager &Audio() { return _audio; }
  FEAPI const audio::AudioManager &Audio() const { return _audio; }

  // Gives access to KVars internals
  FEAPI KVarRegistry &KVarsRegistry() { return _kvars; }
  FEAPI const KVarRegistry &KVarsRegistry() const { return _kvars; }

private:
  memory::MemoryManager &_memoryManager;
  input::InputManager _input;

  audio::AudioManager _audio;
  KVarRegistry _kvars;

  bool _initialized{FeFalse};
};

} // namespace flatearth::modules

#endif // _FLATEARTH_ENGINE_MODULES_CORE_MODULE_HPP
