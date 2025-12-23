#include "VulkanImager.hpp"
#include <vulkan/vulkan_core.h>

namespace flatearth::renderer::vulkan {

FeExpect<void, Error>
ImageManager::CreateImage(Context &ctx, Image &image, VkImageType imageType,
                          uint32 width, uint32 height, VkFormat format,
                          VkImageTiling tiling, VkImageUsageFlags usageFlags,
                          VkMemoryPropertyFlags memoryFlags, bool createView,
                          VkImageAspectFlags aspectFlags) {

  return {};
}

FeExpect<void, Error>
ImageManager::CreateImageView(Context &ctx, Image &image, VkFormat format,
                              VkImageAspectFlags aspectFlags) {
  return {};
}

FeExpect<void, Error> ImageManager::DestroyImage(Context &ctx, Image &image) {
  return {};
}

} // namespace flatearth::renderer::vulkan
