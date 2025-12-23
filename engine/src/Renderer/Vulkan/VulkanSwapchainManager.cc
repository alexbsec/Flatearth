#include "VulkanSwapchainManager.hpp"
#include "Core/FeMemory.hpp"

namespace flatearth::renderer::vulkan {

FeExpect<void, Error>
QuerySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface,
                      SwapchainSupportInfo &outSwapchainInfo,
                      memory::MemoryManager &memManager) {
  FeExpect<void, Error> res;
  // Surface capabilities
  if (res = VkCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
          device, surface, &outSwapchainInfo.capabilities));
      !res.has_value()) {
    FLOG_ERROR("failed to get physical device surface capabilities");
    return FeErr{res.error()};
  }

  // Surface formats
  if (res = VkCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(
          device, surface, &outSwapchainInfo.formatCount, nullptr));
      !res.has_value()) {
    FLOG_ERROR("failed to get physical device surface format count");
    return FeErr{res.error()};
  }

  if (outSwapchainInfo.formatCount != 0) {
    if (outSwapchainInfo.pFormats == nullptr) {
      FLOG_TRACE("before raw alloc of SurfaceFormatKHR");
      outSwapchainInfo.pFormats =
          FeCast<VkSurfaceFormatKHR>(memManager.RawAlloc(
              sizeof(VkSurfaceFormatKHR) * outSwapchainInfo.formatCount,
              alignof(VkSurfaceFormatKHR), memory::Tag::Renderer));
      FLOG_TRACE("after raw alloc of SurfaceFormatKHR");
    }

    if (res = VkCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(
            device, surface, &outSwapchainInfo.formatCount,
            outSwapchainInfo.pFormats));
        !res.has_value()) {
      FLOG_ERROR("failed to get physical device surface formats");
      return FeErr{res.error()};
    }
  }

  // Present mode
  if (res = VkCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(
          device, surface, &outSwapchainInfo.presentModeCount, nullptr));
      !res.has_value()) {
    FLOG_ERROR("failed to get physical device surface present mode count");
    return FeErr{res.error()};
  }

  if (outSwapchainInfo.presentModeCount != 0) {
    if (outSwapchainInfo.pPresentMode == nullptr) {
      outSwapchainInfo.pPresentMode =
          FeCast<VkPresentModeKHR>(memManager.RawAlloc(
              sizeof(VkPresentModeKHR) * outSwapchainInfo.presentModeCount,
              alignof(VkPresentModeKHR), memory::Tag::Renderer));
    }

    if (res = VkCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(
            device, surface, &outSwapchainInfo.presentModeCount,
            outSwapchainInfo.pPresentMode));
        !res.has_value()) {
      FLOG_ERROR("failed to get physical device surface present mode");
      return FeErr{res.error()};
    }
  }

  return {};
}

SwapchainManager::SwapchainManager(memory::MemoryManager &memManager)
    : _memoryManager(memManager) {}

FeExpect<void, Error>
SwapchainManager::CreateSwapchain(Context &ctx, uint32 width, uint32 height) {
  return {};
}

FeExpect<void, Error>
SwapchainManager::RecreateSwapchain(Context &ctx, uint32 width, uint32 height) {
  return {};
}

FeExpect<void, Error> SwapchainManager::DestroySwapchain(Context &ctx) {
  return {};
}

FeExpect<void, Error>
SwapchainManager::AcquireNextImage(Context &ctx, uint64 timeoutNs,
                                   VkSemaphore imageAvailableSemaphore,
                                   VkFence fence, uint32 *outImageIndex) {
  return {};
}

FeExpect<void, Error> SwapchainManager::PresentSwapchain(Context &ctx, VkQueue graphicsQueue,
                                         VkQueue presentQueue,
                                         VkSemaphore renderCompleteSemaphore,
                                         uint32 presentImageIndex) {
  return {};
}

} // namespace flatearth::renderer::vulkan
