#include "VulkanBuffer.hpp"

#include "Core/Logger.hpp"
#include "Renderer/Vulkan/VulkanCommandBufferManager.hpp"

namespace flatearth::renderer::vulkan {

BufferManager::BufferManager(CommandBufferManager &cmdBufferMgr) : _cmdBufferManager(cmdBufferMgr) {
}

FeExpect<void, Error> BufferManager::CreateVulkanBuffer(Context &ctx,
                                                        uint64 size,
                                                        VkBufferUsageFlags usage,
                                                        uint32 memoryPropertyFlags,
                                                        bool bindOnCreate,
                                                        VulkanBuffer *pBuffer) {
  if (pBuffer == nullptr) {
    FLOG_ERROR("cannot create Vulkan buffer on nullptr");
    return FeErr{Error("vulkan buffer is nullptr", ErrorType::NullptrException)};
  }
  ctx.memoryManager.FZeroMemory(pBuffer, sizeof(VulkanBuffer));
  pBuffer->totalSize = size;
  pBuffer->usage = usage;
  pBuffer->memoryPropertyFlags = memoryPropertyFlags;

  VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (auto res = VkCheck(
          vkCreateBuffer(ctx.device.logicalDevice, &bufferInfo, ctx.pAllocator, &pBuffer->handle));
      !res.has_value()) {
    FLOG_ERROR("failed to create buffer");
    return FeErr{res.error()};
  }

  VkMemoryRequirements requirements;
  vkGetBufferMemoryRequirements(ctx.device.logicalDevice, pBuffer->handle, &requirements);
  pBuffer->memoryIndex =
      ctx.FindMemoryIndex(requirements.memoryTypeBits, pBuffer->memoryPropertyFlags);
  if (pBuffer->memoryIndex == -1) {
    FLOG_ERROR("unable to create vulkan buffer because required memory index was "
               "not found");
    return FeErr{Error("failed to find memory index", ErrorType::RendererVulkanError)};
  }

  VkMemoryAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  allocInfo.allocationSize = requirements.size;
  allocInfo.memoryTypeIndex = static_cast<uint32>(pBuffer->memoryIndex);

  VkResult result =
      vkAllocateMemory(ctx.device.logicalDevice, &allocInfo, ctx.pAllocator, &pBuffer->memory);

  if (result != VK_SUCCESS) {
    FLOG_ERROR("unable to create vulkan buffer because the required memory "
               "allocation failed. Error: {}",
               static_cast<uint32>(result));
    return FeErr{Error("failed to allocate buffer", ErrorType::RendererVulkanError)};
  }

  if (!bindOnCreate) {
    return {};
  }

  auto bindRes = BindBuffer(ctx, *pBuffer, 0);
  if (!bindRes.has_value()) {
    FLOG_ERROR("failed to bind buffer on create");
    return FeErr{bindRes.error()};
  }

  return {};
}

void BufferManager::DestroyVulkanBuffer(Context &ctx, VulkanBuffer *pBuffer) {
  if (pBuffer == nullptr) {
    return;
  }

  if (pBuffer->memory != nullptr) {
    vkFreeMemory(ctx.device.logicalDevice, pBuffer->memory, ctx.pAllocator);
    pBuffer->memory = nullptr;
  }

  if (pBuffer->handle) {
    vkDestroyBuffer(ctx.device.logicalDevice, pBuffer->handle, ctx.pAllocator);
    pBuffer->handle = nullptr;
  }

  pBuffer->totalSize = 0;
  pBuffer->isLocked = FeFalse;

  FLOG_DEBUG("vulkan buffer destroyed");
}

FeExpect<void, Error> BufferManager::ResizeBuffer(
    Context &ctx, uint64 newSize, VulkanBuffer &buffer, VkQueue queue, VkCommandPool pool) {
  VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferInfo.size = newSize;
  bufferInfo.usage = buffer.usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkBuffer newBuffer;
  if (auto res = VkCheck(
          vkCreateBuffer(ctx.device.logicalDevice, &bufferInfo, ctx.pAllocator, &newBuffer));
      !res.has_value()) {
    FLOG_ERROR("failed to create new buffer on resize");
    return FeErr{res.error()};
  }

  VkMemoryRequirements requirements;
  vkGetBufferMemoryRequirements(ctx.device.logicalDevice, newBuffer, &requirements);

  VkMemoryAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  allocInfo.allocationSize = requirements.size;
  allocInfo.memoryTypeIndex = static_cast<uint32>(buffer.memoryIndex);

  VkDeviceMemory newMemory;
  VkResult result =
      vkAllocateMemory(ctx.device.logicalDevice, &allocInfo, ctx.pAllocator, &newMemory);
  if (result != VK_SUCCESS) {
    FLOG_ERROR("failed to allocate new memory on resize");
    return FeErr{Error("failed to allocate new memory on resize", ErrorType::RendererVulkanError)};
  }

  if (auto res = VkCheck(vkBindBufferMemory(ctx.device.logicalDevice, newBuffer, newMemory, 0));
      !res.has_value()) {
    FLOG_ERROR("failed to bind new buffer memory");
    return FeErr{res.error()};
  }

  auto copyRes =
      CopyBufferTo(ctx, pool, 0, queue, buffer.handle, 0, newBuffer, 0, buffer.totalSize);
  if (!copyRes.has_value()) {
    FLOG_ERROR("failed to copy buffer");
    return FeErr{copyRes.error()};
  }

  vkDeviceWaitIdle(ctx.device.logicalDevice);

  if (buffer.memory != nullptr) {
    vkFreeMemory(ctx.device.logicalDevice, buffer.memory, ctx.pAllocator);
    buffer.memory = nullptr;
  }

  if (buffer.handle != nullptr) {
    vkDestroyBuffer(ctx.device.logicalDevice, buffer.handle, ctx.pAllocator);
    buffer.handle = nullptr;
  }

  buffer.totalSize = newSize;
  buffer.memory = newMemory;
  buffer.handle = newBuffer;
  return {};
}

FeExpect<void, Error> BufferManager::BindBuffer(Context &ctx, VulkanBuffer &buffer, uint64 offset) {
  if (auto res = VkCheck(
          vkBindBufferMemory(ctx.device.logicalDevice, buffer.handle, buffer.memory, offset));
      !res.has_value()) {
    FLOG_ERROR("failed to bind buffer memory");
    return FeErr{res.error()};
  }

  return {};
}

FeExpect<void *, Error> BufferManager::LockMemory(
    Context &ctx, VulkanBuffer &buffer, uint64 offset, uint64 size, uint32 flags) {
  void *data;
  if (auto res =
          VkCheck(vkMapMemory(ctx.device.logicalDevice, buffer.memory, offset, size, flags, &data));
      !res.has_value()) {
    FLOG_ERROR("failed to map buffer memory");
    return FeErr{res.error()};
  }

  return data;
}

void BufferManager::UnlockMemory(Context &ctx, VulkanBuffer &buffer) {
  vkUnmapMemory(ctx.device.logicalDevice, buffer.memory);
}

FeExpect<void, Error> BufferManager::LoadData(Context &ctx,
                                              VulkanBuffer &buffer,
                                              uint64 offset,
                                              uint64 size,
                                              uint32 flags,
                                              const void *data) {
  void *dataPtr;
  if (auto res = VkCheck(
          vkMapMemory(ctx.device.logicalDevice, buffer.memory, offset, size, flags, &dataPtr));
      !res.has_value()) {
    FLOG_ERROR("failed to map buffer memory");
    return FeErr{res.error()};
  }

  ctx.memoryManager.CopyMemory(dataPtr, data, size);
  vkUnmapMemory(ctx.device.logicalDevice, buffer.memory);
  return {};
}

FeExpect<void, Error> BufferManager::CopyBufferTo(Context &ctx,
                                                  VkCommandPool pool,
                                                  VkFence fence,
                                                  VkQueue queue,
                                                  VkBuffer source,
                                                  uint64 sourceOffset,
                                                  VkBuffer dest,
                                                  uint64 destOffset,
                                                  uint64 size) {
  if (source == VK_NULL_HANDLE || dest == VK_NULL_HANDLE) {
    return FeErr{Error("source or dest buffer is null", ErrorType::NullptrException)};
  }

  if (size == 0) {
    return {};
  }

  // Record + submit + wait in one shot (single-use command buffer)
  return _cmdBufferManager.ImmediateSubmit(ctx, pool, queue, fence, [&](CommandBuffer cmd) {
    VkBufferCopy region{};
    region.srcOffset = sourceOffset;
    region.dstOffset = destOffset;
    region.size = size;

    vkCmdCopyBuffer(cmd.handle, source, dest, 1, &region);
  });
}

} // namespace flatearth::renderer::vulkan
