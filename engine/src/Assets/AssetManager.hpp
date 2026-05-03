#ifndef _FLATEARTH_ENGINE_ASSETS_ASSET_MANAGER_HPP
#define _FLATEARTH_ENGINE_ASSETS_ASSET_MANAGER_HPP

#include "Core/FeMemory.hpp"
#include "Platform/Filesystem.hpp"
#include "Renderer/RendererFrontend.hpp"
#include "Resources/FontCache.hpp"
#include "Resources/ResourceTypes.hpp"
#include "Resources/TextureCache.hpp"

#include <Scene/Components/Sprite.hpp>

namespace flatearth::assets {

class AssetManager {
public:
  explicit AssetManager(memory::MemoryManager &memManager, platform::FileSystem &fs);

  void Initialize(renderer::FrontendRenderer *pRenderer);

  FEAPI FeExpect<resources::TextureHandle, Error>
  LoadTexture(const string &path,
              resources::TextureFilter filter = resources::TextureFilter::Nearest);
  FEAPI void ReleaseTexture(resources::TextureHandle handle);
  FEAPI resources::Texture *GetTexture(resources::TextureHandle handle);

  FEAPI FeExpect<scene::Sprite, Error>
  LoadSprite(stringv path,
             resources::MeshShape shape,
             resources::TextureFilter filter = resources::TextureFilter::Nearest);

  FEAPI FeExpect<scene::Sprite, Error>
  SpriteFromTexture(resources::TextureHandle texHandle,
                    stringv name,
                    resources::MeshShape shape = resources::MeshShape::Quad);

  FEAPI void ReleaseSprite(scene::Sprite &sprite);

  FEAPI FeExpect<resources::FontHandle, Error>
  LoadFont(const string &path, const string &ttfPath, float32 fontSize, int32 atlasSize = 512);
  FEAPI void ReleaseFont(resources::FontHandle handle);
  FEAPI resources::FontAtlas *GetFontAtlas(resources::FontHandle handle);

  FEAPI void Shutdown();

  FEAPI bool Initialized() const;

private:
  resources::TextureCache _textureCache;
  resources::FontCache _fontCache;
  memory::MemoryManager &_memoryManager;
  platform::FileSystem &_fs;
  renderer::FrontendRenderer *_pRenderer;
};

} // namespace flatearth::assets

#endif // _FLATEARTH_ENGINE_ASSETS_ASSET_MANAGER_HPP
