#include "VulkanImager.hpp"
#include "Resources/ResourceTypes.hpp"

#include <vulkan/vulkan_core.h>

namespace flatearth::renderer::vulkan {

FeExpect<void, Error> ImageManager::CreateImage(Context &ctx,
                                                Image &image,
                                                VkImageType imageType,
                                                uint32 width,
                                                uint32 height,
                                                VkFormat format,
                                                VkImageTiling tiling,
                                                VkImageUsageFlags usageFlags,
                                                VkMemoryPropertyFlags memoryFlags,
                                                bool createView,
                                                VkImageAspectFlags aspectFlags,
                                                resources::TextureFilter filter) {
  image.width = width;
  image.height = height;

  VkImageCreateInfo imgCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  imgCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  imgCreateInfo.extent.width = width;
  imgCreateInfo.extent.height = height;
  imgCreateInfo.mipLevels = 4;
  if (filter == resources::TextureFilter::Nearest) {
    imgCreateInfo.mipLevels = 1;
  }

  // TODO: MAKE THESE CONFIGURABLE
  imgCreateInfo.extent.depth = 1;
  imgCreateInfo.arrayLayers = 1;
  imgCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imgCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  // -----------
  imgCreateInfo.format = format;
  imgCreateInfo.tiling = tiling;
  imgCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imgCreateInfo.usage = usageFlags;

  if (auto res = VkCheck(
          vkCreateImage(ctx.device.logicalDevice, &imgCreateInfo, ctx.pAllocator, &image.handle));
      !res.has_value()) {
    FLOG_ERROR("failed to create image");
    return FeErr{res.error()};
  }

  VkMemoryRequirements memReqs;
  vkGetImageMemoryRequirements(ctx.device.logicalDevice, image.handle, &memReqs);

  int32 memType = ctx.FindMemoryIndex(memReqs.memoryTypeBits, memoryFlags);
  if (memType == -1) {
    FLOG_ERROR("required memory type not found: invalid image");
    return FeErr{Error("required memory type was not found", ErrorType::RendererVulkanError)};
  }

  VkMemoryAllocateInfo memInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  memInfo.allocationSize = memReqs.size;
  memInfo.memoryTypeIndex = memType;
  if (auto res = VkCheck(vkAllocateMemory(
          ctx.device.logicalDevice, &memInfo, ctx.pAllocator, &image.deviceMemory));
      !res.has_value()) {
    FLOG_ERROR("failed to allocate memory for image");
    return FeErr{res.error()};
  }

  if (auto res =
          VkCheck(vkBindImageMemory(ctx.device.logicalDevice, image.handle, image.deviceMemory, 0));
      !res.has_value()) {
    FLOG_ERROR("failed to bind image memory");
    return FeErr{res.error()};
  }

  if (!createView) {
    FLOG_INFO("image created successfully. Skipping image view create");
    return {};
  }

  image.view = nullptr;
  auto createRes = CreateImageView(ctx, image, format, aspectFlags);
  if (!createRes.has_value()) {
    FLOG_ERROR("failed to create image view");
    return FeErr{createRes.error()};
  }

  FLOG_INFO("image and image view created successfully");
  return {};
}

FeExpect<void, Error> ImageManager::CreateImageView(Context &ctx,
                                                    Image &image,
                                                    VkFormat format,
                                                    VkImageAspectFlags aspectFlags) {
  VkImageViewCreateInfo viewCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  viewCreateInfo.image = image.handle;
  /* TODO: Make this below configurable */
  viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  /**************************************/
  viewCreateInfo.format = format;
  viewCreateInfo.subresourceRange.aspectMask = aspectFlags;

  /* TODO: Make this below configurable */
  viewCreateInfo.subresourceRange.baseMipLevel = 0;
  viewCreateInfo.subresourceRange.levelCount = 1;
  viewCreateInfo.subresourceRange.baseArrayLayer = 0;
  viewCreateInfo.subresourceRange.layerCount = 1;
  /**************************************/

  if (auto res = VkCheck(vkCreateImageView(
          ctx.device.logicalDevice, &viewCreateInfo, ctx.pAllocator, &image.view));
      !res.has_value()) {
    FLOG_ERROR("failed to create image view");
    return FeErr{res.error()};
  }

  return {};
}

FeExpect<void, Error> ImageManager::TransitionImageLayout(Context &ctx,
                                                          CommandBuffer &cmdBuffer,
                                                          Image &image,
                                                          VkFormat format,
                                                          VkImageLayout oldLayout,
                                                          VkImageLayout newLayout) {
  VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = ctx.device.graphicsQueueIndex;
  barrier.dstQueueFamilyIndex = ctx.device.graphicsQueueIndex;
  barrier.image = image.handle;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags sourceFlags, destFlags;
  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    sourceFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    sourceFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    FLOG_ERROR("unsupported layout transition");
    return FeErr{Error("forbidden layout transition", ErrorType::RendererVulkanError)};
  }

  vkCmdPipelineBarrier(
      cmdBuffer.handle, sourceFlags, destFlags, 0, 0, nullptr, 0, nullptr, 1, &barrier);
  return {};
}

void ImageManager::CopyFromBuffer(Context &ctx,
                                  Image &image,
                                  CommandBuffer &cmdBuffer,
                                  VkBuffer buffer) {
  VkBufferImageCopy region;
  ctx.memoryManager.FZeroMemory(&region, sizeof(VkBufferImageCopy));
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageExtent.width = image.width;
  region.imageExtent.height = image.height;
  region.imageExtent.depth = 1;

  vkCmdCopyBufferToImage(
      cmdBuffer.handle, buffer, image.handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

FeExpect<void, Error> ImageManager::DestroyImage(Context &ctx, Image &image) {
  if (image.view != nullptr) {
    vkDestroyImageView(ctx.device.logicalDevice, image.view, ctx.pAllocator);
    image.view = nullptr;
  }

  if (image.deviceMemory != nullptr) {
    vkFreeMemory(ctx.device.logicalDevice, image.deviceMemory, ctx.pAllocator);
    image.deviceMemory = nullptr;
  }

  if (image.handle != nullptr) {
    vkDestroyImage(ctx.device.logicalDevice, image.handle, ctx.pAllocator);
    image.handle = nullptr;
  }

  FLOG_INFO("image successfully destroyed");
  return {};
}

} // namespace flatearth::renderer::vulkan
