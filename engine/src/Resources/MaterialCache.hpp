#ifndef _FLATEARTH_ENGINE_RESOURCES_MATERIAL_CACHE_HPP
#define _FLATEARTH_ENGINE_RESOURCES_MATERIAL_CACHE_HPP

#include "Core/FeMemory.hpp"
#include "Resources/ResourceCache.hpp"
#include "Resources/ResourceTypes.hpp"

namespace flatearth::renderer { class FrontendRenderer; }

namespace flatearth::resources {

class MaterialCache : public ResourceCache<Material> {
public:
  FEAPI explicit MaterialCache(memory::MemoryManager &memManager,
                               renderer::FrontendRenderer &renderer);

  // Caller resolves TextureHandle → Texture* before calling
  FEAPI FeExpect<MaterialHandle, Error> AcquireMaterial(const string &name, Texture *pTexture);

protected:
  FeExpect<void, Error> Create(Material *pMaterial, uint32 handle, const string &name) override;
  FeExpect<void, Error> Destroy(Material *pMaterial) override;

private:
  renderer::FrontendRenderer &_renderer;
  Texture *_pendingTexture{nullptr};
};

} // namespace flatearth::resources

#endif // _FLATEARTH_ENGINE_RESOURCES_MATERIAL_CACHE_HPP
