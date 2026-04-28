#ifndef _FLATEARTH_ENGINE_CORE_ENGINE_CONTEXT_HPP
#define _FLATEARTH_ENGINE_CORE_ENGINE_CONTEXT_HPP

#include "Modules/AssetModule.hpp"
#include "Modules/CoreModule.hpp"
#include "Modules/GameModule.hpp"

namespace flatearth {

struct EngineContext {
  modules::Core &core;
  modules::Assets &assets;
  modules::Project &project;
  uint32 drawCallCount{0};

  explicit EngineContext(modules::Core &core, modules::Assets &assets, modules::Project &game)
      : core(core), assets(assets), project(game) {}
};

} // namespace flatearth

#endif // _FLATEARTH_ENGINE_CORE_ENGINE_CONTEXT_HPP
