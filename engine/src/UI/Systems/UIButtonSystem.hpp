#ifndef _FLATEARTH_ENGINE_UI_SYSTEMS_UI_BUTTON_SYSTEM_HPP
#define _FLATEARTH_ENGINE_UI_SYSTEMS_UI_BUTTON_SYSTEM_HPP

#include "Core/CoreTypes.hpp"
#include "Core/Input.hpp"
#include "ECS/SystemInterface.hpp"

namespace flatearth::ui::systems {

class UIButtonSystem : public ecs::ISystem {
public:
  explicit UIButtonSystem(input::InputManager &input, EngineState *pEngState);
  void Update(ecs::Registry &registry, float32 deltaTime) override;

private:
  input::InputManager &_input;
  EngineState *_pEngState;
};

} // namespace flatearth::ui::systems

#endif // _FLATEARTH_ENGINE_UI_SYSTEMS_UI_BUTTON_SYSTEM_HPP
