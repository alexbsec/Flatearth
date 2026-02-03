#include "VulkanFrameBufferManager.hpp"

#include "Core/FeMemory.hpp"
#include "Core/Logger.hpp"

#include <vulkan/vulkan_core.h>

namespace flatearth::renderer::vulkan {

FrameBufferManager::FrameBufferManager(memory::MemoryManager &memManager)
    : _memoryManager(memManager) {
}

FrameBufferManager::~FrameBufferManager() {
  FLOG_INFO("frame buffer exitted gracefully");
}

FeExpect<void, Error> FrameBufferManager::CreateFrameBuffer(Context &ctx,
                                                            Renderpass &renderpass,
                                                            FrameBuffer *pFrameBuffer,
                                                            uint32 width,
                                                            uint32 height,
                                                            uint32 attachmentCount,
                                                            VkImageView *pAttachments) {
  if (pFrameBuffer == nullptr) {
    FLOG_ERROR("cannot create on nullptr framebuffer");
    return FeErr{Error("attempt to create framebuffer on nullptr", ErrorType::NullptrException)};
  }

  pFrameBuffer->pAttachments = FeCast<VkImageView>(_memoryManager.RawAlloc(
      sizeof(VkImageView) * attachmentCount, alignof(VkImageView), memory::Tag::Renderer));
  _lastAttachmentCount = attachmentCount;

  for (uint32 i = 0; i < attachmentCount; i++) {
    pFrameBuffer->pAttachments[i] = pAttachments[i];
  }

  pFrameBuffer->attachmentCount = attachmentCount;
  pFrameBuffer->pRenderpass = &renderpass;

  constexpr uint32 cLayers = 1;

  VkFramebufferCreateInfo createInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
  createInfo.renderPass = renderpass.handle;
  createInfo.attachmentCount = attachmentCount;
  createInfo.pAttachments = pFrameBuffer->pAttachments;
  createInfo.width = width;
  createInfo.height = height;
  createInfo.layers = cLayers;

  if (auto res = VkCheck(vkCreateFramebuffer(
          ctx.device.logicalDevice, &createInfo, ctx.pAllocator, &pFrameBuffer->handle));
      !res.has_value()) {
    FLOG_ERROR("failed to create vulkan framebuffer");
    return FeErr{res.error()};
  }

  return {};
}

FeExpect<void, Error> FrameBufferManager::DestroyFrameBuffer(Context &ctx,
                                                             FrameBuffer *pFrameBuffer) {
  vkDeviceWaitIdle(ctx.device.logicalDevice);

  vkDestroyFramebuffer(ctx.device.logicalDevice, pFrameBuffer->handle, ctx.pAllocator);

  if (pFrameBuffer->pAttachments != nullptr) {
    _memoryManager.RawFree(pFrameBuffer->pAttachments,
                           sizeof(VkImage) * pFrameBuffer->attachmentCount,
                           memory::Tag::Renderer);
    pFrameBuffer->pAttachments = nullptr;
  }

  pFrameBuffer->handle = nullptr;
  pFrameBuffer->attachmentCount = 0;
  pFrameBuffer->pRenderpass = nullptr;
  return {};
}

} // namespace flatearth::renderer::vulkan
