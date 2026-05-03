#ifndef _FLATEARTH_ENGINE_RENDERER_VULKAN_SHADERS_VULKAN_SHADER_HPP
#define _FLATEARTH_ENGINE_RENDERER_VULKAN_SHADERS_VULKAN_SHADER_HPP

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

  FeExpect<bool, Error> CreateShader(Context &ctx,
                                     ObjectShader *pObjShader,
                                     stringv name,
                                     uint32 pushConstantSize,
                                     VkShaderStageFlags pushConstantStageFlags);
  void DestroyShader(Context &ctx, ObjectShader *pObjectShader);
  void UpdateShader(Context &ctx,
                    ObjectShader &shader,
                    const void *data,
                    uint32 size,
                    VkShaderStageFlags stageFlags);

  void UseShader(Context &ctx, ObjectShader &objShader);
  FeExpect<void, Error> UpdateGlobalState(Context &ctx, ObjectShader &objShader);

  FeExpect<void, Error>
  AcquireTextureResources(Context &ctx, ObjectShader &objShader, TextureData *pTextureData);

private:
  memory::MemoryManager &_memoryManager;
  BufferManager &_bufferManager;
  PipelineManager _pipelineManager;
};

} // namespace flatearth::renderer::vulkan::shaders

#endif // _FLATEARTH_ENGINE_RENDERER_VULKAN_SHADERS_VULKAN_SHADER_HPP
