#ifndef _FLATEARTH_ENGINE_ASSETS_TILEMAP_LOADER_HPP
#define _FLATEARTH_ENGINE_ASSETS_TILEMAP_LOADER_HPP

#include "Defines.hpp"
#include "Error.hpp"
#include "Resources/Tilemap.hpp"

namespace flatearth::assets {

class AssetManager;

class TilemapLoader {
public:
  explicit TilemapLoader(AssetManager &assetManager);
  FEAPI FeExpect<resources::Tilemap, Error> Load(stringv tmxPath);

private:
  AssetManager &_assetManager;
};

} // namespace flatearth::assets

#endif // _FLATEARTH_ENGINE_ASSETS_TILEMAP_LOADER_HPP
