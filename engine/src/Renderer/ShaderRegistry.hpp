#ifndef _FLATEARTH_ENGINE_RENDERER_SHADERS_SHADER_REGISTRY_HPP
#define _FLATEARTH_ENGINE_RENDERER_SHADERS_SHADER_REGISTRY_HPP

#include "Containers/HashMap.hpp"
#include "Core/FeMemory.hpp"
#include "Renderer/RendererTypes.hpp"

namespace flatearth::renderer {

class ShaderRegistry {
public:
  explicit ShaderRegistry(memory::MemoryManager &);
  FeExpect<ShaderHandle, Error>
  NewShader(stringv name, ShaderInterpreter interpreter = ShaderInterpreter::Vulkan);

  Shader *RetrieveShader(stringv name);
  const Shader *RetrieveShader(stringv name) const;
  Shader *RetrieveShader(ShaderHandle handle);
  const Shader *RetrieveShader(ShaderHandle handle) const;

  void DestroyShader(stringv name);

private:
  containers::HashMap<string, ShaderHandle> _handlesMap;
  containers::HashMap<ShaderHandle, FePtr<Shader>> _shadersMap;
  memory::MemoryManager &_memoryManager;
  ShaderHandle _nextHandle{0};
};

} // namespace flatearth::renderer

#endif // _FLATEARTH_ENGINE_RENDERER_SHADERS_SHADER_REGISTRY_HPP
