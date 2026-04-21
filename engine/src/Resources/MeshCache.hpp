#ifndef _FLATEARTH_ENGINE_RESOURCES_MESH_CACHE_HPP
#define _FLATEARTH_ENGINE_RESOURCES_MESH_CACHE_HPP

#include "Core/FeMemory.hpp"
#include "Resources/ResourceCache.hpp"
#include "Resources/ResourceTypes.hpp"

namespace flatearth::renderer {
class FrontendRenderer;
}

namespace flatearth::resources {

class MeshCache : public ResourceCache<Mesh> {
public:
  FEAPI explicit MeshCache(memory::MemoryManager &memManager, renderer::FrontendRenderer &renderer);

  FEAPI FeExpect<MeshHandle, Error> AcquireMesh(MeshShape shape);

protected:
  FeExpect<void, Error> Create(Mesh *pMesh, uint32 handle, const string &name) override;
  FeExpect<void, Error> Destroy(Mesh *pMesh) override;

private:
  renderer::FrontendRenderer &_renderer;
  MeshShape _pendingShape{MeshShape::Quad};
};

} // namespace flatearth::resources

#endif // _FLATEARTH_ENGINE_RESOURCES_MESH_CACHE_HPP
