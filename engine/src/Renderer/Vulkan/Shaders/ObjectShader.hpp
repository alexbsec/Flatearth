#ifndef _FLATEARTH_ENGINE_RENDERER_VULKAN_SHADERS_OBJECT_SHADER_HPP
#define _FLATEARTH_ENGINE_RENDERER_VULKAN_SHADERS_OBJECT_SHADER_HPP

#include "Renderer/Vulkan/VulkanTypes.hpp"

namespace flatearth::renderer::vulkan::shaders {

class VulkanShader {
public:
  explicit VulkanShader(memory::MemoryManager &memManager);
  ~VulkanShader();

  FeExpect<bool, Error> CreateObjectShader(Context &ctx,
                                           ObjectShader *pObjShader);
  void DestroyObjectShader(Context &ctx, ObjectShader *pObjShader);

  void UseShader(Context &ctx, ObjectShader &objShader);

private:
  memory::MemoryManager &_memoryManager;
};

} // namespace flatearth::renderer::vulkan::shaders

#endif // _FLATEARTH_ENGINE_RENDERER_VULKAN_TYPES_HPP
