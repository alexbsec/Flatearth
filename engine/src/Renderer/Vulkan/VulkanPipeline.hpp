#ifndef _FLATEARTH_ENGINE_RENDERER_VULKAN_PIPELINE_HPP
#define _FLATEARTH_ENGINE_RENDERER_VULKAN_PIPELINE_HPP

#include "VulkanTypes.hpp"

#include <vulkan/vulkan_core.h>

namespace flatearth::renderer::vulkan {

class PipelineManager {
public:
  FeExpect<void, Error> CreateGraphicsPipeline(Context &ctx,
                                               Renderpass *pRenderpass,
                                               uint32 attributeCount,
                                               VkVertexInputAttributeDescription *pAttributes,
                                               uint32 descriptorSetLayoutCount,
                                               VkDescriptorSetLayout *pDescriptorLayouts,
                                               uint32 stageCount,
                                               VkPipelineShaderStageCreateInfo *pStages,
                                               VkViewport viewport,
                                               VkRect2D scissor,
                                               bool isWireframe,
                                               Pipeline *pPipeline);

  void DestroyGraphicsPipeline(Context &ctx, Pipeline *pPipeline);

  void BindPipeline(CommandBuffer &cmdBuffer, VkPipelineBindPoint bindPoint, Pipeline &pipeline);
};

} // namespace flatearth::renderer::vulkan

#endif // _FLATEARTH_ENGINE_RENDERER_VULKAN_PIPELINE_HPP
