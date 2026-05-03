#ifndef _FLATEARTH_ENGINE_UI_SYSTEMS_UI_SYSTEM_HPP
#define _FLATEARTH_ENGINE_UI_SYSTEMS_UI_SYSTEM_HPP

#include "ECS/SystemInterface.hpp"

namespace flatearth::ui::systems {

class UISystem : public ecs::ISystem {
public:
  void Update(ecs::Registry &, float32) override;
};

} // namespace flatearth::ui::systems

#endif // _FLATEARTH_ENGINE_UI_SYSTEMS_UI_SYSTEM_HPP
