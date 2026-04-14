#ifndef _FLATEARTH_ENGINE_RESOURCES_TEXTURE_SYSTEM_HPP
#define _FLATEARTH_ENGINE_RESOURCES_TEXTURE_SYSTEM_HPP

#include "Containers/HashMap.hpp"
#include "Core/FeMemory.hpp"
#include "Platform/Filesystem.hpp"
#include "Renderer/RendererFrontend.hpp"
#include "Resources/ResourceTypes.hpp"

namespace flatearth::resources {

struct TextureEntry {
  Texture texture{};
  string path{};
  uint32 refCount{0};
};

class TextureSystem {
public:
  FEAPI explicit TextureSystem(memory::MemoryManager &memManager,
                               renderer::FrontendRenderer &renderer,
                               platform::FileSystem &fs);
  FEAPI FeExpect<TextureHandle, Error> AcquireTexture(const string &path);
  FEAPI void ReleaseTexture(TextureHandle handle);
  FEAPI Texture *GetTexture(TextureHandle handle);

private:
  FeExpect<void, Error> LoadTexture(const string &path, Texture *pTexture);

private:
  memory::MemoryManager &_memoryManager;
  renderer::FrontendRenderer &_frontendRenderer;
  platform::FileSystem &_filesystem;

  TextureHandle _nextHandle{0};
  containers::HashMap<string, TextureHandle> _pathHandleMap;
  containers::HashMap<TextureHandle, TextureEntry> _handleEntryMap;
};

} // namespace flatearth::resources

#endif // _FLATEARTH_ENGINE_RESOURCES_TEXTURE_SYSTEM_HPP
