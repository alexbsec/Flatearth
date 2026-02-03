#ifndef _FLATEARTH_ENGINE_RENDERER_VULKAN_FRAME_BUFFER_MANAGER_HPP
#define _FLATEARTH_ENGINE_RENDERER_VULKAN_FRAME_BUFFER_MANAGER_HPP

#include "Core/FeMemory.hpp"
#include "VulkanTypes.hpp"

namespace flatearth::renderer::vulkan {

class FrameBufferManager {
public:
  explicit FrameBufferManager(memory::MemoryManager &memManager);
  ~FrameBufferManager();

  FeExpect<void, Error> CreateFrameBuffer(Context &ctx,
                                          Renderpass &renderpass,
                                          FrameBuffer *pFrameBuffer,
                                          uint32 width,
                                          uint32 height,
                                          uint32 attachmentCount,
                                          VkImageView *pAttachments);

  FeExpect<void, Error> DestroyFrameBuffer(Context &ctx, FrameBuffer *pFrameBuffer);

private:
  memory::MemoryManager &_memoryManager;
  uint32 _lastAttachmentCount{0};
};

} // namespace flatearth::renderer::vulkan

#endif // _FLATEARTH_ENGINE_RENDERER_VULKAN_FRAME_BUFFER_MANAGER_HPP
