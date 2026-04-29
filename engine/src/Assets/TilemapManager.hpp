#ifndef _FLATEARTH_ENGINE_ASSETS_TILEMAP_MANAGER_HPP
#define _FLATEARTH_ENGINE_ASSETS_TILEMAP_MANAGER_HPP

#include "Containers/HashMap.hpp"
#include "Core/FeMemory.hpp"
#include "Defines.hpp"
#include "Error.hpp"
#include "ECS/ECSTypes.hpp"
#include "Scene/Components/Sprite.hpp"
#include "Scene/Scene.hpp"

namespace flatearth::ecs { class Registry; }

namespace flatearth::assets {

class AssetManager;

class TilemapManager {
public:
  explicit TilemapManager(memory::MemoryManager &memManager,
                          AssetManager &assetManager,
                          ecs::Registry &registry);

  FEAPI FeExpect<ecs::EntityId, Error> Load(stringv tmxPath, scene::SceneId sceneId);
  FEAPI void Unload(stringv tmxPath);

private:
  AssetManager   &_assetManager;
  ecs::Registry  &_registry;
  containers::HashMap<string, scene::Sprite> _baseSprites;
};

} // namespace flatearth::assets

#endif // _FLATEARTH_ENGINE_ASSETS_TILEMAP_MANAGER_HPP
