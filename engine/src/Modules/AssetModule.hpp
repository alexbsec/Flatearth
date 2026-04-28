#ifndef _FLATEARTH_ENGINE_MODULES_ASSET_MODULE_HPP
#define _FLATEARTH_ENGINE_MODULES_ASSET_MODULE_HPP

#include "Assets/AssetManager.hpp"
#include "Assets/PrefabManager.hpp"
#include "Assets/TilemapManager.hpp"
#include "Platform/Filesystem.hpp"
#include "Renderer/RendererFrontend.hpp"
namespace flatearth::modules {

class Assets {
public:
  explicit Assets(memory::MemoryManager &mm, platform::FileSystem &fs, ecs::Registry &reg)
      : _manager(mm, fs), _tilemap(mm, _manager, reg), _prefab(mm) {}

  FeExpect<bool, Error> Initialize(renderer::FrontendRenderer *pRenderer);
  void Shutdown();

  FEAPI assets::AssetManager &Manager() { return _manager; }
  FEAPI const assets::AssetManager &Manager() const { return _manager; }

  FEAPI assets::TilemapManager &Tilemap() { return _tilemap; }
  FEAPI const assets::TilemapManager &Tilemape() const { return _tilemap; }

  FEAPI assets::PrefabManager &Prefab() { return _prefab; }
  FEAPI const assets::PrefabManager &Prefab() const { return _prefab; }

private:
  assets::AssetManager _manager;
  assets::TilemapManager _tilemap;
  assets::PrefabManager _prefab;

  bool _initialized{FeFalse};
};

} // namespace flatearth::modules

#endif // _FLATEARTH_ENGINE_MODULES_ASSET_MODULE_HPP
