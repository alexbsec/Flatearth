#ifndef _FLATEARTH_ENGINE_RESOURCES_MATERIAL_SYSTEM_HPP
#define _FLATEARTH_ENGINE_RESOURCES_MATERIAL_SYSTEM_HPP

#include "Core/FeMemory.hpp"
#include "Renderer/RendererFrontend.hpp"
#include "Resources/ResourceSystem.hpp"
#include "Resources/ResourceTypes.hpp"
#include "Resources/TextureSystem.hpp"

namespace flatearth::resources {

class MaterialSystem : public ResourceSystem<Material> {
public:
  FEAPI explicit MaterialSystem(memory::MemoryManager &memManager,
                                renderer::FrontendRenderer &renderer,
                                TextureSystem &textureSystem);

  FEAPI FeExpect<MaterialHandle, Error> AcquireMaterial(const string &name,
                                                        TextureHandle texHandle);

protected:
  FeExpect<void, Error> Create(Material *pMaterial, uint32 handle, const string &name) override;
  FeExpect<void, Error> Destroy(Material *pMaterial) override;

private:
  renderer::FrontendRenderer &_renderer;
  TextureSystem &_textureSystem;
  TextureHandle _pendingTexHandle{0};
};

} // namespace flatearth::resources

#endif // _FLATEARTH_ENGINE_RESOURCES_MATERIAL_SYSTEM_HPP
