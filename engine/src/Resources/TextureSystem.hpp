#ifndef _FLATEARTH_ENGINE_RESOURCES_TEXTURE_SYSTEM_HPP
#define _FLATEARTH_ENGINE_RESOURCES_TEXTURE_SYSTEM_HPP

#include "Core/FeMemory.hpp"
#include "Platform/Filesystem.hpp"
#include "Renderer/RendererFrontend.hpp"
#include "Resources/ResourceSystem.hpp"
#include "Resources/ResourceTypes.hpp"

namespace flatearth::resources {

class TextureSystem : public ResourceSystem<Texture> {
public:
  FEAPI explicit TextureSystem(memory::MemoryManager &memManager,
                               renderer::FrontendRenderer &renderer,
                               platform::FileSystem &fs);

  FEAPI FeExpect<TextureHandle, Error>
  AcquireTexture(const string &path, TextureFilter filter = TextureFilter::Nearest);

protected:
  FeExpect<void, Error> Create(Texture *pTexture, uint32 handle, const string &path) override;
  FeExpect<void, Error> Destroy(Texture *pTexture) override;

private:
  TextureFilter _filterToUse{TextureFilter::Nearest};
  renderer::FrontendRenderer &_renderer;
  platform::FileSystem &_fs;
};

} // namespace flatearth::resources

#endif // _FLATEARTH_ENGINE_RESOURCES_TEXTURE_SYSTEM_HPP
