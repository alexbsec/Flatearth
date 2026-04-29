#include "TilesetLoader.hpp"

#include "Assets/AssetManager.hpp"
#include "tinyxml2.h"

using namespace flatearth::resources;

namespace txml2 = tinyxml2;

namespace flatearth::assets {

TilesetLoader::TilesetLoader(AssetManager &am) : _assetManager(am) {
}

FeExpect<Tileset, Error> TilesetLoader::Load(stringv tsxPath) {
  txml2::XMLDocument doc;
  if (doc.LoadFile(tsxPath.data()) != txml2::XML_SUCCESS) {
    return FeErr{Error("failed to parse tsx file", ErrorType::FileReadError)};
  }

  auto *root = doc.FirstChildElement("tileset");

  Tileset ts;
  ts.name = root->Attribute("name");
  ts.tileWidth = root->UnsignedAttribute("tilewidth");
  ts.tileHeight = root->UnsignedAttribute("tileheight");
  ts.tileCount = root->UnsignedAttribute("tilecount");
  ts.columns   = root->UnsignedAttribute("columns");

  auto *imageElem = root->FirstChildElement("image");
  ts.imageWidth  = imageElem->UnsignedAttribute("width");
  ts.imageHeight = imageElem->UnsignedAttribute("height");

  namespace stdfs = std::filesystem;
  const string cImagePath = (stdfs::path(tsxPath).parent_path() / imageElem->Attribute("source")).string();
  auto texRes = _assetManager.LoadTexture(cImagePath).or_error("TilesetLoader: failed to load texture");
  if (texRes.errored()) {
    return FeErr{texRes.error()};
  }

  ts.imagePath = cImagePath;
  ts.texture   = texRes.value();
  ts.tiles.resize(ts.tileCount);
  float32 uvW = (float32)ts.tileWidth / (float32)ts.imageWidth;
  float32 uvH = (float32)ts.tileHeight / (float32)ts.imageHeight;

  for (uint32 id = 0; id < ts.tileCount; id++) {
    uint32 col = id % ts.columns;
    uint32 row = id / ts.columns;
    ts.tiles[id] = {
      .id       = id,
      .uvOffset = {col * uvW, (row + 1) * uvH},  // start at bottom edge of tile row
      .uvScale  = {uvW, -uvH},                    // sample upward to flip vertically
    };
  }

  return std::move(ts);
}

} // namespace flatearth::assets
