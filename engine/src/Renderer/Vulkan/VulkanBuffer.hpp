#ifndef _FLATEARTH_ENGINE_RENDERER_VULKAN_BUFFER_HPP
#define _FLATEARTH_ENGINE_RENDERER_VULKAN_BUFFER_HPP

#include "Renderer/Vulkan/VulkanCommandBufferManager.hpp"
#include "VulkanTypes.hpp"

namespace flatearth::renderer::vulkan {

class BufferManager {
public:
  explicit BufferManager(CommandBufferManager &cmdBufferMgr);
  FeExpect<void, Error> CreateVulkanBuffer(Context &ctx,
                                           uint64 size,
                                           VkBufferUsageFlags usage,
                                           uint32 memoryPropertyFlags,
                                           bool bindOnCreate,
                                           VulkanBuffer *pBuffer);

  void DestroyVulkanBuffer(Context &ctx, VulkanBuffer *pBuffer);

  FeExpect<void, Error> ResizeBuffer(
      Context &ctx, uint64 newSize, VulkanBuffer &buffer, VkQueue queue, VkCommandPool pool);

  FeExpect<void, Error> BindBuffer(Context &ctx, VulkanBuffer &buffer, uint64 offset);

  FeExpect<void *, Error>
  LockMemory(Context &ctx, VulkanBuffer &buffer, uint64 offset, uint64 size, uint32 flags);
  void UnlockMemory(Context &ctx, VulkanBuffer &buffer);

  FeExpect<void, Error> LoadData(Context &ctx,
                                 VulkanBuffer &buffer,
                                 uint64 offset,
                                 uint64 size,
                                 uint32 flags,
                                 const void *data);

  FeExpect<void, Error> CopyBufferTo(Context &ctx,
                                     VkCommandPool pool,
                                     VkFence fence,
                                     VkQueue queue,
                                     VkBuffer source,
                                     uint64 sourceOffset,
                                     VkBuffer dest,
                                     uint64 destOffset,
                                     uint64 size);

private:
  CommandBufferManager &_cmdBufferManager;
};

} // namespace flatearth::renderer::vulkan

#endif // _FLATEARTH_ENGINE_RENDERER_VULKAN_BUFFER_HPP
