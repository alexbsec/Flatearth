#ifndef _FLATEARTH_ENGINE_ECS_SYSTEM_INTERFACE_HPP
#define _FLATEARTH_ENGINE_ECS_SYSTEM_INTERFACE_HPP

#include "ECS/Registry.hpp"

namespace flatearth::ecs {

class ISystem {
public:
  virtual ~ISystem() = default;
  virtual void Update(Registry &registry, float32 deltaTime) = 0;
};

}

#endif // _FLATEARTH_ENGINE_ECS_SYSTEM_INTERFACE_HPP
