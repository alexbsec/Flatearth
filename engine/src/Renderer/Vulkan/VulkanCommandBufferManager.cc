#include "VulkanCommandBufferManager.hpp"
#include "Core/Logger.hpp"
#include "Renderer/Vulkan/VulkanTypes.hpp"
#include <vulkan/vulkan_core.h>

namespace flatearth::renderer::vulkan {

CommandBufferManager::CommandBufferManager(memory::MemoryManager &memManager)
    : _memoryManager(memManager) {}

CommandBufferManager::~CommandBufferManager() {
  FLOG_INFO("command buffer exitted successfully");
}

FeExpect<void, Error> CommandBufferManager::CreateBuffers(Context &ctx) {
  if (ctx.graphicsCommandBuffer.Empty()) {
    ctx.graphicsCommandBuffer.Reserve(ctx.swapchain.imageCount);
    for (uint32 i = 0; i < ctx.swapchain.imageCount; i++) {
      _memoryManager.ZeroMemory(&ctx.graphicsCommandBuffer[i],
                                sizeof(CommandBuffer));
    }
  }

  for (uint32 i = 0; i < ctx.swapchain.imageCount; i++) {
    if (ctx.graphicsCommandBuffer[i].state != CmdBufferState::NotAllocated) {
      auto freeRes = FreeBuffer(ctx, &ctx.graphicsCommandBuffer[i],
                                ctx.device.graphicsCommandPool);
      if (!freeRes.has_value()) {
        FLOG_ERROR("failed to free buffer at index {}", i);
        return FeErr{freeRes.error()};
      }
    }
    _memoryManager.ZeroMemory(&ctx.graphicsCommandBuffer[i],
                              sizeof(CommandBuffer));
    auto allocRes = AllocateBuffer(ctx, &ctx.graphicsCommandBuffer[i],
                                   ctx.device.graphicsCommandPool, FeTrue);
    if (!allocRes.has_value()) {
      FLOG_ERROR("failed to allocate buffer at inidex {}", i);
      return FeErr{allocRes.error()};
    }
  }

  return {};
}

FeExpect<void, Error> CommandBufferManager::DestroyBuffers(Context &ctx) {
  if (ctx.graphicsCommandBuffer.Empty()) {
    FLOG_DEBUG("no buffer to destroy");
    return {};
  }

  for (uint32 i = 0; i < ctx.swapchain.imageCount; i++) {
    if (ctx.graphicsCommandBuffer[i].handle == nullptr) {
      continue;
    }

    auto freeRes = FreeBuffer(ctx, &ctx.graphicsCommandBuffer[i],
                              ctx.device.graphicsCommandPool);
    if (!freeRes.has_value()) {
      FLOG_ERROR("failed to free buffer at index {}", i);
      return FeErr{freeRes.error()};
    }
  }

  return {};
}

FeExpect<void, Error>
CommandBufferManager::AllocateBuffer(Context &ctx, CommandBuffer *pCmdBuffer,
                                     VkCommandPool pool, bool isPrimary) {
  // just to make sure
  _memoryManager.ZeroMemory(pCmdBuffer, sizeof(CommandBuffer));

  VkCommandBufferAllocateInfo allocInfo = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocInfo.commandPool = pool;
  allocInfo.level = isPrimary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY
                              : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
  allocInfo.commandBufferCount = 1;
  allocInfo.pNext = nullptr;

  pCmdBuffer->state = CmdBufferState::NotAllocated;
  if (auto res = VkCheck(vkAllocateCommandBuffers(
          ctx.device.logicalDevice, &allocInfo, &pCmdBuffer->handle));
      !res.has_value()) {
    FLOG_ERROR("failed to allocate command buffers");
    return FeErr{res.error()};
  }
  pCmdBuffer->state = CmdBufferState::Ready;
  return {};
}

FeExpect<void, Error>
CommandBufferManager::FreeBuffer(Context &ctx, CommandBuffer *pCmdBuffer,
                                 VkCommandPool pool) {
  constexpr uint32 cCmdBufferCount = 1;
  vkFreeCommandBuffers(ctx.device.logicalDevice, pool, cCmdBufferCount,
                       &pCmdBuffer->handle);
  return {};
}

void CommandBufferManager::BeginBuffer(Context &ctx, CommandBuffer &cmdBuffer,
                                       bool isSingleUse,
                                       bool isRenderpassContinue,
                                       bool isSimultaneousUse) {}

void CommandBufferManager::EndBuffer(Context &ctx, CommandBuffer &cmdBuffer) {}

void CommandBufferManager::ResetBuffer(Context &ctx, CommandBuffer &cmdBuffer) {
}

} // namespace flatearth::renderer::vulkan
