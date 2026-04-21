#ifndef _FLATEARTH_ENGINE_RESOURCES_TEXTURE_CACHE_HPP
#define _FLATEARTH_ENGINE_RESOURCES_TEXTURE_CACHE_HPP

#include "Core/FeMemory.hpp"
#include "Platform/Filesystem.hpp"
#include "Renderer/RendererFrontend.hpp"
#include "Resources/ResourceCache.hpp"
#include "Resources/ResourceTypes.hpp"

namespace flatearth::resources {

class TextureCache : public ResourceCache<Texture> {
public:
  FEAPI explicit TextureCache(memory::MemoryManager &memManager, platform::FileSystem &fs);

  void Initialize(renderer::FrontendRenderer *pRenderer);

  FEAPI FeExpect<TextureHandle, Error>
  AcquireTexture(const string &path, TextureFilter filter = TextureFilter::Nearest);

  bool Initialized() const;

protected:
  FeExpect<void, Error> Create(Texture *pTexture, uint32 handle, const string &path) override;
  FeExpect<void, Error> Destroy(Texture *pTexture) override;

private:
  TextureFilter _filterToUse{TextureFilter::Nearest};
  platform::FileSystem &_fs;
  renderer::FrontendRenderer *_pRenderer;
};

} // namespace flatearth::resources

#endif // _FLATEARTH_ENGINE_RESOURCES_TEXTURE_CACHE_HPP
