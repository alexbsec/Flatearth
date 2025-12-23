#ifndef _FLATEARTH_ENGINE_RENDERER_VULKAN_IMAGER_HPP
#define _FLATEARTH_ENGINE_RENDERER_VULKAN_IMAGER_HPP

#include "Error.hpp"
#include "Renderer/Vulkan/VulkanTypes.hpp"

namespace flatearth::renderer::vulkan {

class ImageManager {
public:
  FeExpect<void, Error>
  CreateImage(Context &ctx, Image &image, VkImageType imageType, uint32 width,
              uint32 height, VkFormat format, VkImageTiling tiling,
              VkImageUsageFlags usageFlags, VkMemoryPropertyFlags memoryFlags,
              bool createView, VkImageAspectFlags aspectFlags);

  FeExpect<void, Error> CreateImageView(Context &ctx, Image &image,
                                        VkFormat format,
                                        VkImageAspectFlags aspectFlags);

  FeExpect<void, Error> DestroyImage(Context &ctx, Image &image);
};

} // namespace flatearth::renderer::vulkan

#endif // _FLATEARTH_ENGINE_RENDERER_VULKAN_IMAGER_HPP
