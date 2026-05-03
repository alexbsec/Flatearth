#ifndef _FLATEARTH_ENGINE_RESOURCES_FONT_CACHE_HPP
#define _FLATEARTH_ENGINE_RESOURCES_FONT_CACHE_HPP

#include "Platform/Filesystem.hpp"
#include "Renderer/RendererFrontend.hpp"
#include "Resources/ResourceCache.hpp"
#include "Resources/ResourceTypes.hpp"

namespace flatearth::resources {

class FontCache : public ResourceCache<FontAtlas> {
public:
  explicit FontCache(memory::MemoryManager &, platform::FileSystem &);

  void Initialize(renderer::FrontendRenderer *pRenderer);

  FEAPI FeExpect<FontHandle, Error>
  AcquireFont(const string &name, const string &ttfPath, float32 fontSize, int32 atlasSize = 512);

  bool Initialized() const;

protected:
  FeExpect<void, Error> Create(FontAtlas *, uint32 handle, const string &name) override;
  FeExpect<void, Error> Destroy(FontAtlas *) override;

private:
  platform::FileSystem &_filesystem;
  renderer::FrontendRenderer *_pRenderer{nullptr};
  string _pendingTtfPath{};
  float32 _pendingFontSize{16.0f};
  int32 _pendingAtlasSize{512};
};

} // namespace flatearth::resources

#endif // _FLATEARTH_ENGINE_RESOURCES_FONT_CACHE_HPP
