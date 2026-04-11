#ifndef _FLATEARTH_ENGINE_RENDERER_VULKAN_SHADERS_OBJECT_SHADER_HPP
#define _FLATEARTH_ENGINE_RENDERER_VULKAN_SHADERS_OBJECT_SHADER_HPP

#include "Renderer/Vulkan/VulkanBuffer.hpp"
#include "Renderer/Vulkan/VulkanPipeline.hpp"
#include "Renderer/Vulkan/VulkanTypes.hpp"

namespace flatearth::renderer::vulkan::shaders {

enum class DescriptorBinding {
  Global,
  Texture,
};

FeExpect<void, Error>
MakeLayoutBinding(const Context &ctx, ObjectShader *pObjectShader, DescriptorBinding layout);

FeExpect<void, Error>
MakeDescriptorPool(const Context &ctx, ObjectShader *pObjectShader, DescriptorBinding binding);

class VulkanShader {
public:
  explicit VulkanShader(memory::MemoryManager &memManager, BufferManager &bufferManager);
  ~VulkanShader();

  FeExpect<bool, Error> CreateObjectShader(Context &ctx, ObjectShader *pObjShader);
  void DestroyObjectShader(Context &ctx, ObjectShader *pObjShader);

  void UseShader(Context &ctx, ObjectShader &objShader);
  FeExpect<void, Error> UpdateGlobalState(Context &ctx, ObjectShader &objShader);

  // temp method
  void UpdateObject(Context &ctx, ObjectShader &objShader, math::Mat4D model);

  FeExpect<void, Error>
  AcquireTextureResources(Context &ctx, ObjectShader &objShader, TextureData *pTextureData);

private:
  memory::MemoryManager &_memoryManager;
  BufferManager &_bufferManager;
  PipelineManager _pipelineManager;
};

} // namespace flatearth::renderer::vulkan::shaders

#endif // _FLATEARTH_ENGINE_RENDERER_VULKAN_TYPES_HPP
