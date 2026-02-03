#ifndef _FLATEARTH_ENGINE_RENDERER_VULKAN_SWAPCHAIN_MANAGER_HPP
#define _FLATEARTH_ENGINE_RENDERER_VULKAN_SWAPCHAIN_MANAGER_HPP

#include "Core/FeMemory.hpp"
#include "Renderer/Vulkan/VulkanFrameBufferManager.hpp"
#include "Renderer/Vulkan/VulkanImager.hpp"
#include "Renderer/Vulkan/VulkanTypes.hpp"

namespace flatearth::renderer::vulkan {

FeExpect<void, Error> QuerySwapchainSupport(VkPhysicalDevice device,
                                            VkSurfaceKHR surface,
                                            SwapchainSupportInfo& outInfo,
                                            memory::MemoryManager& memManager);

class SwapchainManager {
public:
  explicit SwapchainManager(memory::MemoryManager& memManager, ImageManager& imgManager);

  FeExpect<void, Error>
  CreateSwapchain(Context& ctx, Swapchain* pSwapchain, uint32 width, uint32 height);
  FeExpect<bool, Error> RecreateSwapchain(Context& ctx, uint32& width, uint32& height);
  FeExpect<void, Error> DestroySwapchain(Context& ctx, Swapchain* pSwapchain);

  FeExpect<bool, Error> AcquireNextImage(Context& ctx,
                                         Swapchain& swapchain,
                                         uint64 timeoutNs,
                                         VkSemaphore imageAvailableSemaphore,
                                         VkFence fence,
                                         uint32* outImageIndex);

  FeExpect<void, Error> PresentSwapchain(Context& ctx,
                                         Swapchain& swapchain,
                                         VkQueue graphicsQueue,
                                         VkQueue presentQueue,
                                         VkSemaphore renderCompleteSemaphore,
                                         uint32 presentImageIndex);

  FeExpect<void, Error>
  RegenerateFrameBuffer(Context& ctx, Swapchain* pSwapchain, Renderpass* pRenderpass);

private:
  FeExpect<void, Error>
  CreateLogic(Context& ctx, Swapchain* pSwapchain, uint32 width, uint32 height);
  FeExpect<void, Error> DestroyLogic(Context& ctx, Swapchain* pSwapchain);
  FeExpect<void, Error>
  RecreateLogic(Context& ctx, Swapchain* pSwapchain, uint32 width, uint32 height);

private:
  memory::MemoryManager& _memoryManager;
  ImageManager& _imageManager;

  FrameBufferManager _frameBufferManager;
};

} // namespace flatearth::renderer::vulkan

#endif // _FLATEARTH_ENGINE_RENDERER_VULKAN_SWAPCHAIN_MANAGER_HPP
