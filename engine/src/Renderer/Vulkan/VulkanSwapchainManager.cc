#include "VulkanSwapchainManager.hpp"
#include "Core/FeMemory.hpp"
#include "Renderer/Vulkan/VulkanTypes.hpp"
#include <vulkan/vulkan_core.h>

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

FeExpect<void, Error> SwapchainManager::CreateSwapchain(Context &ctx,
                                                        Swapchain *pSwapchain,
                                                        uint32 width,
                                                        uint32 height) {
  return CreateLogic(ctx, pSwapchain, width, height);
}

FeExpect<void, Error> SwapchainManager::RecreateSwapchain(Context &ctx,
                                                          Swapchain *pSwapchain,
                                                          uint32 width,
                                                          uint32 height) {
  auto destroyRes = DestroyLogic(ctx, pSwapchain);
  if (!destroyRes.has_value()) {
    FLOG_ERROR("failed to destroy swapchain in recreate process");
    return FeErr{destroyRes.error()};
  }

  auto createRes = CreateLogic(ctx, pSwapchain, width, height);
  if (!createRes.has_value()) {
    FLOG_ERROR("failed to create swapchain in recreate process");
    return FeErr{createRes.error()};
  }

  return {};
}

FeExpect<void, Error>
SwapchainManager::DestroySwapchain(Context &ctx, Swapchain *pSwapchain) {
  return DestroyLogic(ctx, pSwapchain);
}

FeExpect<void, Error>
SwapchainManager::AcquireNextImage(Context &ctx, uint64 timeoutNs,
                                   VkSemaphore imageAvailableSemaphore,
                                   VkFence fence, uint32 *outImageIndex) {
  return {};
}

FeExpect<void, Error> SwapchainManager::PresentSwapchain(
    Context &ctx, VkQueue graphicsQueue, VkQueue presentQueue,
    VkSemaphore renderCompleteSemaphore, uint32 presentImageIndex) {
  return {};
}

FeExpect<void, Error> SwapchainManager::CreateLogic(Context &ctx,
                                                    Swapchain *pSwapchain,
                                                    uint32 width,
                                                    uint32 height) {
  if (pSwapchain == nullptr) {
    FLOG_ERROR("cannot create on nullptr swapchain");
    return FeErr{
        Error("swapchain creation failed because swapchain pointer was null",
              ErrorType::NullptrException)};
  }

  if (ctx.device.swapchainSupportInfo.formatCount == 0) {
    FLOG_ERROR("no available format on renderer context");
    return FeErr{Error("swapchain creation failed because of lack of format",
                       ErrorType::RendererVulkanError)};
  }

  bool found = FeFalse;
  for (uint32 i = 0; i < ctx.device.swapchainSupportInfo.formatCount; i++) {
    VkSurfaceFormatKHR format = ctx.device.swapchainSupportInfo.pFormats[i];
    // preferred formats
    if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      pSwapchain->imageFormat = format;
      found = FeTrue;
      break;
    }
  }

  if (!found) {
    // Take first if preferred is not found
    pSwapchain->imageFormat = ctx.device.swapchainSupportInfo.pFormats[0];
  }

  if (ctx.device.swapchainSupportInfo.presentModeCount == 0) {
    FLOG_WARN("no available present mode was found on renderer context. "
              "Defaulting to VK_PRESENT_MODE_FIFO_KHR");
  }

  VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
  for (uint32 i = 0; i < ctx.device.swapchainSupportInfo.presentModeCount;
       i++) {
    VkPresentModeKHR mode = ctx.device.swapchainSupportInfo.pPresentMode[i];
    if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
      presentMode = mode;
      break;
    }
  }

  auto queryRes =
      QuerySwapchainSupport(ctx.device.physicalDevice, ctx.surface,
                            ctx.device.swapchainSupportInfo, _memoryManager);
  if (!queryRes.has_value()) {
    FLOG_ERROR("swapchain support query failed");
    return FeErr{queryRes.error()};
  }

  VkExtent2D swapchainExtent = VkExtent2D{width, height};
  if (ctx.device.swapchainSupportInfo.capabilities.currentExtent.width !=
      UINT32_MAX) {
    swapchainExtent =
        ctx.device.swapchainSupportInfo.capabilities.currentExtent;
  }

  VkExtent2D min = ctx.device.swapchainSupportInfo.capabilities.minImageExtent;
  VkExtent2D max = ctx.device.swapchainSupportInfo.capabilities.maxImageExtent;
  swapchainExtent.width =
      FECLAMP(swapchainExtent.height, min.height, max.height);
  swapchainExtent.height =
      FECLAMP(swapchainExtent.height, min.height, max.height);

  uint32 imageCount =
      ctx.device.swapchainSupportInfo.capabilities.minImageCount + 1;
  if (ctx.device.swapchainSupportInfo.capabilities.maxImageCount > 0 &&
      imageCount > ctx.device.swapchainSupportInfo.capabilities.maxImageCount) {
    imageCount = ctx.device.swapchainSupportInfo.capabilities.maxImageCount;
  }

  pSwapchain->maxFrames = imageCount - 1;

  VkSwapchainCreateInfoKHR createInfo = {
      VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
  createInfo.surface = ctx.surface;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = pSwapchain->imageFormat.format;
  createInfo.imageColorSpace = pSwapchain->imageFormat.colorSpace;
  createInfo.imageExtent = swapchainExtent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  if (ctx.device.graphicsQueueIndex != ctx.device.presentQueueIndex) {
    uint32 queueFamilyIndices[]{
        (uint32)ctx.device.graphicsQueueIndex,
        (uint32)ctx.device.presentQueueIndex,
    };
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
  }

  createInfo.preTransform =
      ctx.device.swapchainSupportInfo.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = FeTrue;
  createInfo.oldSwapchain = nullptr;

  if (auto res =
          VkCheck(vkCreateSwapchainKHR(ctx.device.logicalDevice, &createInfo,
                                       ctx.pAllocator, &pSwapchain->handle));
      !res.has_value()) {
    FLOG_ERROR("failed to create vulkan swapchain");
    return FeErr{res.error()};
  }

  ctx.currentFrame = 0;
  pSwapchain->imageCount = 0;
  if (auto res = VkCheck(
          vkGetSwapchainImagesKHR(ctx.device.logicalDevice, pSwapchain->handle,
                                  &pSwapchain->imageCount, nullptr));
      !res.has_value()) {
    FLOG_ERROR("failed to get swapchain images");
    return FeErr{res.error()};
  }

  if (pSwapchain->pViews == nullptr) {
    pSwapchain->pViews = FeCast<VkImageView>(
        _memoryManager.RawAlloc(sizeof(VkImageView) * pSwapchain->imageCount,
                                alignof(VkImageView), memory::Tag::Renderer));
  }

  if (auto res = VkCheck(vkGetSwapchainImagesKHR(
          ctx.device.logicalDevice, pSwapchain->handle, &pSwapchain->imageCount,
          pSwapchain->pImages));
      !res.has_value()) {
    FLOG_ERROR("failed to get swapchain images");
    return FeErr{res.error()};
  }

  // Setting views
  for (uint32 i = 0; i < pSwapchain->imageCount; i++) {
    VkImageViewCreateInfo viewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image = pSwapchain->pImages[i];
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = pSwapchain->imageFormat.format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (auto res =
            VkCheck(vkCreateImageView(ctx.device.logicalDevice, &viewInfo,
                                      ctx.pAllocator, &pSwapchain->pViews[i]));
        !res.has_value()) {
      FLOG_ERROR("failed to create image view for index {}", i);
      return FeErr{res.error()};
    }
  }

  if (auto res = DetectDeviceDepthFormat(ctx.device); !res.has_value()) {
    FLOG_WARN("failed to find a supported depth format");
    ctx.device.depthFormat = VK_FORMAT_UNDEFINED;
  }

  // create image
  // CreateImage()

  FLOG_INFO("swapchain creation flow ended successfully");
  return {};
}

FeExpect<void, Error> SwapchainManager::DestroyLogic(Context &ctx,
                                                     Swapchain *pSwapchain) {
  if (pSwapchain == nullptr) {
    FLOG_ERROR("cannot destroy on nullptr swapchain");
    return FeErr{
        Error("swapchain destruction failed because swapchain pointer was null",
              ErrorType::NullptrException)};
  }

  vkDeviceWaitIdle(ctx.device.logicalDevice);

  // Destroy image
  // DestroyImage()

  for (uint32 i = 0; i < pSwapchain->imageCount; i++) {
    vkDestroyImageView(ctx.device.logicalDevice, pSwapchain->pViews[i],
                       ctx.pAllocator);
  }

  if (pSwapchain->pViews != nullptr) {
    _memoryManager.RawFree(pSwapchain->pViews,
                           sizeof(VkImageView) * pSwapchain->imageCount,
                           memory::Tag::Renderer);
    pSwapchain->pViews = nullptr;
  }

  if (pSwapchain->pImages != nullptr) {
    _memoryManager.RawFree(pSwapchain->pImages,
                           sizeof(VkImage) * pSwapchain->imageCount,
                           memory::Tag::Renderer);
    pSwapchain->pImages = nullptr;
  }

  if (pSwapchain->handle != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(ctx.device.logicalDevice, pSwapchain->handle,
                          ctx.pAllocator);
    pSwapchain->handle = VK_NULL_HANDLE;
  }

  pSwapchain->imageCount = 0;
  FLOG_INFO("swapchain successfully destroyed");
  return {};
}

} // namespace flatearth::renderer::vulkan
