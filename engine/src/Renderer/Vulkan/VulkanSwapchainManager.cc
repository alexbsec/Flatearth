#include "VulkanSwapchainManager.hpp"
#include "Core/FeMemory.hpp"
#include "Core/Logger.hpp"
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

  if (outSwapchainInfo.formatCount > 0) {
    if (outSwapchainInfo.formatCount > outSwapchainInfo.formatsCapacity) {
      if (outSwapchainInfo.pFormats) {
        memManager.RawFree(outSwapchainInfo.pFormats,
                           sizeof(VkSurfaceFormatKHR) *
                               outSwapchainInfo.formatsCapacity,
                           memory::Tag::Renderer);
      }
      outSwapchainInfo.pFormats =
          FeCast<VkSurfaceFormatKHR>(memManager.RawAlloc(
              sizeof(VkSurfaceFormatKHR) * outSwapchainInfo.formatCount,
              alignof(VkSurfaceFormatKHR), memory::Tag::Renderer));
      outSwapchainInfo.formatsCapacity = outSwapchainInfo.formatCount;
    }

    if (res = VkCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(
            device, surface, &outSwapchainInfo.formatCount,
            outSwapchainInfo.pFormats));
        !res.has_value()) {
      FLOG_ERROR("failed to get phyiscal device surface formats");
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
    if (outSwapchainInfo.presentModeCount >
        outSwapchainInfo.presentModesCapacity) {
      if (outSwapchainInfo.pPresentMode) {
        memManager.RawFree(outSwapchainInfo.pPresentMode,
                           sizeof(VkPresentModeKHR) *
                               outSwapchainInfo.presentModesCapacity,
                           memory::Tag::Renderer);
      }
      outSwapchainInfo.pPresentMode =
          FeCast<VkPresentModeKHR>(memManager.RawAlloc(
              sizeof(VkPresentModeKHR) * outSwapchainInfo.presentModeCount,
              alignof(VkPresentModeKHR), memory::Tag::Renderer));
      outSwapchainInfo.presentModesCapacity = outSwapchainInfo.presentModeCount;
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

SwapchainManager::SwapchainManager(memory::MemoryManager &memManager,
                                   ImageManager &imgManager)
    : _memoryManager(memManager), _imageManager(imgManager),
      _frameBufferManager(memManager) {}

FeExpect<void, Error> SwapchainManager::CreateSwapchain(Context &ctx,
                                                        Swapchain *pSwapchain,
                                                        uint32 width,
                                                        uint32 height) {
  return CreateLogic(ctx, pSwapchain, width, height);
}

FeExpect<bool, Error> SwapchainManager::RecreateSwapchain(Context &ctx,
                                                          uint32 &width,
                                                          uint32 &height) {
  if (ctx.recreatingSwapchain) {
    FLOG_WARN("called when already recreating swapchain");
    return FeFalse;
  }

  // Mark as recreating
  ctx.recreatingSwapchain = FeTrue;
  vkDeviceWaitIdle(ctx.device.logicalDevice);

  // Clear images
  for (uint32 i = 0; i < ctx.swapchain.imageCount; i++) {
    ctx.imagesInFlight[i] = nullptr;
  }

  auto queryRes =
      QuerySwapchainSupport(ctx.device.physicalDevice, ctx.surface,
                            ctx.device.swapchainSupportInfo, _memoryManager);
  if (!queryRes.has_value()) {
    FLOG_ERROR("failed to query swapchain support");
    return FeErr{queryRes.error()};
  }

  auto detectRes = DetectDeviceDepthFormat(ctx.device);
  if (!detectRes.has_value()) {
    FLOG_ERROR("failed to detect device depth format");
    return FeErr{detectRes.error()};
  }

  auto recreateRes = RecreateLogic(ctx, &ctx.swapchain, width, height);
  if (!recreateRes.has_value()) {
    FLOG_ERROR("swapchain recreation logic failed");
    return FeErr{recreateRes.error()};
  }

  ctx.framebufferWidth = width;
  ctx.framebufferHeight = height;
  ctx.mainRenderpass.width = width;
  ctx.mainRenderpass.height = height;
  width = height = 0;

  ctx.framebufferSizeLastGeneration = ctx.framebufferSizeGeneration;

  // TODO: clean swapchain and command buffers

  ctx.mainRenderpass.x = 0;
  ctx.mainRenderpass.y = 0;
  ctx.mainRenderpass.width = ctx.framebufferWidth;
  ctx.mainRenderpass.height = ctx.framebufferHeight;

  auto fbRes = RegenerateFrameBuffer(ctx, &ctx.swapchain, &ctx.mainRenderpass);
  if (!fbRes.has_value()) {
    return FeErr{fbRes.error()};
  }

  ctx.recreatingSwapchain = FeFalse;
  FLOG_INFO("swapchain recreated successfully");
  return {};
}

FeExpect<void, Error>
SwapchainManager::DestroySwapchain(Context &ctx, Swapchain *pSwapchain) {
  return DestroyLogic(ctx, pSwapchain);
}

FeExpect<bool, Error> SwapchainManager::AcquireNextImage(
    Context &ctx, Swapchain &swapchain, uint64 timeoutNs,
    VkSemaphore imageAvailableSemaphore, VkFence fence, uint32 *outImageIndex) {

  VkResult result = vkAcquireNextImageKHR(
      ctx.device.logicalDevice, swapchain.handle, timeoutNs,
      imageAvailableSemaphore, fence, outImageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    // This errors happens when resizing goes wrong for example
    // We simply recreate the swapchain in this case
    auto res = RecreateLogic(ctx, &swapchain, ctx.framebufferWidth,
                             ctx.framebufferHeight);
    if (!res.has_value()) {
      FLOG_ERROR("failed to recreate swapchain after VK_ERROR_OUT_OF_DATE_KHR");
      return FeErr{res.error()};
    }
    return FeFalse;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    // These are only Vulkan errors and we simply log a fatal error
    // for the engine
    FLOG_FATAL("VulkanBackend::SwapchainAcquireNextImage(): Failed to acquire "
               "swapchain image");
    return FeErr{Error("failed to acquire next image from swapchain",
                       ErrorType::RendererVulkanError)};
  }

  // If hits here, it means everything is ok
  return {};
}

FeExpect<void, Error> SwapchainManager::PresentSwapchain(
    Context &ctx, Swapchain &swapchain, VkQueue graphicsQueue, VkQueue presentQueue,
    VkSemaphore renderCompleteSemaphore, uint32 presentImageIndex) {

  // Return the image to the swapchain for presentation
  VkPresentInfoKHR presentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pNext = nullptr;
  presentInfo.pWaitSemaphores = &renderCompleteSemaphore;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &swapchain.handle;
  presentInfo.pImageIndices = &presentImageIndex;
  presentInfo.pResults = nullptr;

  VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    // Must recreate
    auto res = RecreateSwapchain(ctx, ctx.framebufferWidth, ctx.framebufferHeight);
    if (!res.has_value()) {
      FLOG_ERROR("failed to recreate swapchain after present returned out of "
                 "date or suboptimal");
      return FeErr{res.error()};
    }
  } else if (result != VK_SUCCESS) {
    FLOG_FATAL("Failed to present swapchain image");
    return FeErr{Error("failed to present swapchain image",
                       ErrorType::RendererVulkanError)};
  }

  // Increment and loop the index
  ctx.currentFrame = (ctx.currentFrame + 1) % swapchain.maxFrames;
  ctx.imageIndex = (ctx.imageIndex + 1) % swapchain.imageCount;
  return {};
}

FeExpect<void, Error>
SwapchainManager::RegenerateFrameBuffer(Context &ctx, Swapchain *pSwapchain,
                                        Renderpass *pRenderpass) {
  if (pSwapchain == nullptr) {
    return FeErr{Error("cannot regenerate frame buffer with nullptr swapchain",
                       ErrorType::NullptrException)};
  }

  if (pRenderpass == nullptr) {
    return FeErr{Error("cannot regenerate frame buffer with nullptr renderpass",
                       ErrorType::NullptrException)};
  }

  // TODO: remove this hardcoded part
  constexpr uint32 cAttachmentCount = 2;
  for (uint32 i = 0; i < pSwapchain->imageCount; i++) {
    uint32 attachmentCount = cAttachmentCount;
    VkImageView attachments[] = {
        pSwapchain->pViews[i],
        pSwapchain->depthAttachment.view,
    };

    auto createRes = _frameBufferManager.CreateFrameBuffer(
        ctx, *pRenderpass, &ctx.swapchain.framebuffers[i], ctx.framebufferWidth,
        ctx.framebufferHeight, attachmentCount, attachments);

    if (!createRes.has_value()) {
      FLOG_ERROR("failed to create framebuffer at index {}", i);
      return FeErr{createRes.error()};
    }
  }

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

  if (width == 0 || height == 0) {
    FLOG_FATAL("width or height is passed as zero. width={}, height={}", width,
               height);
    return FeErr{Error("cannot create swapchain with 0 width or height",
                       ErrorType::RendererVulkanError)};
  }

  auto queryRes =
      QuerySwapchainSupport(ctx.device.physicalDevice, ctx.surface,
                            ctx.device.swapchainSupportInfo, _memoryManager);
  if (!queryRes.has_value()) {
    FLOG_ERROR("swapchain support query failed");
    return FeErr{queryRes.error()};
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

  VkExtent2D swapchainExtent = VkExtent2D{width, height};
  if (ctx.device.swapchainSupportInfo.capabilities.currentExtent.width !=
      UINT32_MAX) {
    swapchainExtent =
        ctx.device.swapchainSupportInfo.capabilities.currentExtent;
  }

  VkExtent2D min = ctx.device.swapchainSupportInfo.capabilities.minImageExtent;
  VkExtent2D max = ctx.device.swapchainSupportInfo.capabilities.maxImageExtent;
  swapchainExtent.width = FECLAMP(swapchainExtent.width, min.width, max.width);
  swapchainExtent.height =
      FECLAMP(swapchainExtent.height, min.height, max.height);

  pSwapchain->widthExtent = swapchainExtent.width;
  pSwapchain->heightExtent = swapchainExtent.height;

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

  uint32 queueFamilyIndices[2] = {(uint32)ctx.device.graphicsQueueIndex,
                                  (uint32)ctx.device.presentQueueIndex};
  if (ctx.device.graphicsQueueIndex != ctx.device.presentQueueIndex) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
  }

  createInfo.preTransform =
      ctx.device.swapchainSupportInfo.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = VK_NULL_HANDLE;

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
    FLOG_ERROR("failed to get swapchain image count");
    return FeErr{res.error()};
  }

  if (pSwapchain->pImages != nullptr) {
    _memoryManager.RawFree(pSwapchain->pImages,
                           sizeof(VkImage) * pSwapchain->imageCount,
                           memory::Tag::Renderer);
  }

  pSwapchain->pImages = FeCast<VkImage>(
      _memoryManager.RawAlloc(sizeof(VkImage) * pSwapchain->imageCount,
                              alignof(VkImage), memory::Tag::Renderer));

  pSwapchain->imagesCapacity = pSwapchain->imageCount;

  // Ensure capacity for views
  if (pSwapchain->pViews != nullptr) {
    _memoryManager.RawFree(pSwapchain->pViews,
                           sizeof(VkImageView) * pSwapchain->imageCount,
                           memory::Tag::Renderer);
  }

  pSwapchain->pViews = FeCast<VkImageView>(
      _memoryManager.RawAlloc(sizeof(VkImageView) * pSwapchain->imageCount,
                              alignof(VkImageView), memory::Tag::Renderer));

  pSwapchain->viewsCapacity = pSwapchain->imageCount;

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
  auto imageRes = _imageManager.CreateImage(
      ctx, pSwapchain->depthAttachment, VK_IMAGE_TYPE_2D, swapchainExtent.width,
      swapchainExtent.height, ctx.device.depthFormat, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, FeTrue, VK_IMAGE_ASPECT_DEPTH_BIT);
  if (!imageRes.has_value()) {
    FLOG_ERROR("failed to create depth attachment image");
    return FeErr{imageRes.error()};
  }

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

  for (uint32 i = 0; i < pSwapchain->imageCount; i++) {
    auto delRes = _frameBufferManager.DestroyFrameBuffer(
        ctx, &pSwapchain->framebuffers[i]);
    if (!delRes.has_value()) {
      FLOG_ERROR("failed to delete frame buffer at index {}", i);
      return FeErr{delRes.error()};
    }
  }
  pSwapchain->framebuffers.Clear();

  // Destroy image
  auto imageRes = _imageManager.DestroyImage(ctx, pSwapchain->depthAttachment);
  if (!imageRes.has_value()) {
    FLOG_ERROR("failed to destroy depth attachment image");
    return FeErr{imageRes.error()};
  }

  for (uint32 i = 0; i < pSwapchain->imageCount; i++) {
    vkDestroyImageView(ctx.device.logicalDevice, pSwapchain->pViews[i],
                       ctx.pAllocator);
  }

  if (pSwapchain->pViews != nullptr) {
    _memoryManager.RawFree(pSwapchain->pViews,
                           sizeof(VkImageView) * pSwapchain->viewsCapacity,
                           memory::Tag::Renderer);
    pSwapchain->pViews = nullptr;
  }

  if (pSwapchain->pImages != nullptr) {
    _memoryManager.RawFree(pSwapchain->pImages,
                           sizeof(VkImage) * pSwapchain->imagesCapacity,
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

FeExpect<void, Error> SwapchainManager::RecreateLogic(Context &ctx,
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

} // namespace flatearth::renderer::vulkan
