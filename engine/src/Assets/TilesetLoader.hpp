#ifndef _FLATEARTH_ENGINE_ASSETS_TILESET_LOADER_HPP
#define _FLATEARTH_ENGINE_ASSETS_TILESET_LOADER_HPP

#include "Assets/AssetManager.hpp"
#include "Resources/Tileset.hpp"

namespace flatearth::assets {

class TilesetLoader {
public:
  explicit TilesetLoader(assets::AssetManager &am);
  FeExpect<resources::Tileset, Error> Load(stringv tsxPath);

private:
  assets::AssetManager &_assetManager;
};

}

#endif // _FLATEARTH_ENGINE_ASSETS_TILESET_LOADER_HPP
