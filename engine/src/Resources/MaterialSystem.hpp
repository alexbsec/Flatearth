#ifndef _FLATEARTH_ENGINE_RESOURCES_MATERIAL_SYSTEM_HPP
#define _FLATEARTH_ENGINE_RESOURCES_MATERIAL_SYSTEM_HPP

#include "Containers/DArray.hpp"
#include "Containers/HashMap.hpp"
#include "Core/FeMemory.hpp"
#include "Renderer/RendererFrontend.hpp"
#include "Resources/ResourceTypes.hpp"
#include "Resources/TextureSystem.hpp"

namespace flatearth::resources {

struct MaterialEntry {
  Material material{};
  uint32 refCount{0};
};

class MaterialSystem {
public:
  FEAPI explicit MaterialSystem(memory::MemoryManager &memManager,
                                renderer::FrontendRenderer &renderer,
                                TextureSystem &textureSystem);
  FEAPI FeExpect<MaterialHandle, Error> AcquireMaterial(const string &name,
                                                        TextureHandle texHandle);
  FEAPI void ReleaseMaterial(MaterialHandle handle);
  FEAPI Material *GetMaterial(MaterialHandle handle);
  FEAPI void Shutdown();

private:
  memory::MemoryManager &_memoryManager;
  renderer::FrontendRenderer &_frontendRenderer;
  TextureSystem &_textureSystem;

  MaterialHandle _nextHandle{0};
  containers::HashMap<string, MaterialHandle> _nameHandleMap;
  containers::HashMap<MaterialHandle, MaterialEntry> _handleEntryMap;
  containers::DArray<MaterialHandle> _activeHandles;
};

} // namespace flatearth::resources

#endif // _FLATEARTH_ENGINE_RESOURCES_MATERIAL_SYSTEM_HPP
