#ifndef _FLATEARTH_ENGINE_ASSETS_TILEMAP_MANAGER_HPP
#define _FLATEARTH_ENGINE_ASSETS_TILEMAP_MANAGER_HPP

#include "Defines.hpp"
#include "Error.hpp"
#include "ECS/ECSTypes.hpp"

namespace flatearth::scene { struct Scene; }
namespace flatearth::ecs   { class Registry; }

namespace flatearth::assets {

class AssetManager;

class TilemapManager {
public:
  explicit TilemapManager(AssetManager &assetManager, ecs::Registry &registry);

  // Loads a .tmx file, creates all tile and collision entities, registers them
  // in the scene for automatic cleanup on SceneManager::Unload.
  // Returns the root tilemap entity.
  FEAPI FeExpect<ecs::EntityId, Error> Load(stringv tmxPath, scene::Scene *scene);

private:
  AssetManager   &_assetManager;
  ecs::Registry  &_registry;
};

} // namespace flatearth::assets

#endif // _FLATEARTH_ENGINE_ASSETS_TILEMAP_MANAGER_HPP
