#ifndef _FLATEARTH_ENGINE_RESOURCES_MATERIAL_CACHE_HPP
#define _FLATEARTH_ENGINE_RESOURCES_MATERIAL_CACHE_HPP

#include "Core/FeMemory.hpp"
#include "Renderer/RendererFrontend.hpp"
#include "Resources/ResourceCache.hpp"
#include "Resources/ResourceTypes.hpp"
#include "Resources/TextureCache.hpp"

namespace flatearth::resources {

class MaterialCache : public ResourceCache<Material> {
public:
  FEAPI explicit MaterialCache(memory::MemoryManager &memManager,
                               renderer::FrontendRenderer &renderer,
                               TextureCache &textureCache);

  FEAPI FeExpect<MaterialHandle, Error> AcquireMaterial(const string &name,
                                                        TextureHandle texHandle);

protected:
  FeExpect<void, Error> Create(Material *pMaterial, uint32 handle, const string &name) override;
  FeExpect<void, Error> Destroy(Material *pMaterial) override;

private:
  renderer::FrontendRenderer &_renderer;
  TextureCache &_textureCache;
  TextureHandle _pendingTexHandle{0};
};

} // namespace flatearth::resources

#endif // _FLATEARTH_ENGINE_RESOURCES_MATERIAL_CACHE_HPP
