#include "VulkanPipeline.hpp"
#include "Math/MathTypes.hpp"
#include "Renderer/Vulkan/VulkanTypes.hpp"
#include <vulkan/vulkan_core.h>

namespace flatearth::renderer::vulkan {

FeExpect<void, Error> PipelineManager::CreateGraphicsPipeline(
    Context &ctx, Renderpass *pRenderpass, uint32 attributeCount,
    VkVertexInputAttributeDescription *pAttributes,
    uint32 descriptorSetLayoutCount, VkDescriptorSetLayout *pDescriptorLayouts,
    uint32 stageCount, VkPipelineShaderStageCreateInfo *pStages,
    VkViewport viewport, VkRect2D scissor, bool isWireframe,
    Pipeline *pPipeline) {
  if (pRenderpass == nullptr) {
    FLOG_ERROR("cannot create graphics pipeline with nullptr renderpass");
    return FeErr{Error("renderpass is nullptr", ErrorType::NullptrException)};
  }

  if (pPipeline == nullptr) {
    FLOG_ERROR("cannot create graphics pipeline on nullptr pipeline");
    return FeErr{
        Error("pipeline object is nullptr", ErrorType::NullptrException)};
  }

  VkPipelineViewportStateCreateInfo viewportState = {
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  // Rasterizer
  VkPipelineRasterizationStateCreateInfo rasterizerInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
  rasterizerInfo.depthClampEnable = VK_FALSE;
  rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
  rasterizerInfo.polygonMode =
      isWireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
  rasterizerInfo.lineWidth = 1.0f;
  rasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizerInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizerInfo.depthBiasEnable = VK_FALSE;
  rasterizerInfo.depthBiasConstantFactor = 0.0f;
  rasterizerInfo.depthBiasClamp = 0.0f;
  rasterizerInfo.depthBiasSlopeFactor = 0.0f;

  // Multisampling
  VkPipelineMultisampleStateCreateInfo multisamplingInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
  multisamplingInfo.sampleShadingEnable = VK_FALSE;
  multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisamplingInfo.minSampleShading = 1.0f;
  multisamplingInfo.pSampleMask = nullptr;
  multisamplingInfo.alphaToCoverageEnable = VK_FALSE;
  multisamplingInfo.alphaToOneEnable = VK_FALSE;

  // Depth and stencil
  VkPipelineDepthStencilStateCreateInfo stencilInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
  stencilInfo.depthTestEnable = VK_TRUE;
  stencilInfo.depthWriteEnable = VK_TRUE;
  stencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
  stencilInfo.depthBoundsTestEnable = VK_FALSE;
  stencilInfo.stencilTestEnable = VK_FALSE;

  VkPipelineColorBlendAttachmentState colorBlendState;
  ctx.memoryManager.FZeroMemory(&colorBlendState,
                                sizeof(VkPipelineColorBlendAttachmentState));
  colorBlendState.blendEnable = VK_TRUE;
  colorBlendState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendState.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendState.alphaBlendOp = VK_BLEND_OP_ADD;
  colorBlendState.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

  VkPipelineColorBlendStateCreateInfo colorBlendInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
  colorBlendInfo.logicOpEnable = VK_FALSE;
  colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
  colorBlendInfo.attachmentCount = 1;
  colorBlendInfo.pAttachments = &colorBlendState;

  // Dynamic state
  const uint32 cDynamicCount = 3;
  std::array<VkDynamicState, cDynamicCount> dynamicStates = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
      VK_DYNAMIC_STATE_LINE_WIDTH,
  };

  VkPipelineDynamicStateCreateInfo dynamicStateInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
  };
  dynamicStateInfo.dynamicStateCount = cDynamicCount;
  dynamicStateInfo.pDynamicStates = dynamicStates.data();

  // Vertex input
  VkVertexInputBindingDescription bindingDescription;
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(math::Vertex3D);
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  // Attributes
  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount = attributeCount;
  vertexInputInfo.pVertexAttributeDescriptions = pAttributes;

  // Input assembly
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
  inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

  // Pipeline layout
  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  pipelineLayoutInfo.setLayoutCount = descriptorSetLayoutCount;
  pipelineLayoutInfo.pSetLayouts = pDescriptorLayouts;
  auto res = VkCheck(vkCreatePipelineLayout(ctx.device.logicalDevice,
                                            &pipelineLayoutInfo, ctx.pAllocator,
                                            &pPipeline->layout));
  if (!res.has_value()) {
    FLOG_ERROR("failed to create pipeline layout");
    return FeErr{res.error()};
  }

  VkGraphicsPipelineCreateInfo graphicsInfo = {
      VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
  graphicsInfo.stageCount = stageCount;
  graphicsInfo.pStages = pStages;
  graphicsInfo.pVertexInputState = &vertexInputInfo;
  graphicsInfo.pInputAssemblyState = &inputAssemblyInfo;
  graphicsInfo.pViewportState = &viewportState;
  graphicsInfo.pRasterizationState = &rasterizerInfo;
  graphicsInfo.pMultisampleState = &multisamplingInfo;
  graphicsInfo.pDepthStencilState = &stencilInfo;
  graphicsInfo.pDynamicState = &dynamicStateInfo;
  graphicsInfo.pColorBlendState = &colorBlendInfo;
  graphicsInfo.pTessellationState = nullptr;

  graphicsInfo.layout = pPipeline->layout;
  graphicsInfo.renderPass = pRenderpass->handle;
  graphicsInfo.subpass = 0;
  graphicsInfo.basePipelineHandle = VK_NULL_HANDLE;
  graphicsInfo.basePipelineIndex = -1;

  VkResult result = vkCreateGraphicsPipelines(
      ctx.device.logicalDevice, VK_NULL_HANDLE, 1, &graphicsInfo,
      ctx.pAllocator, &pPipeline->handle);
  if (result == VK_SUCCESS) {
    FLOG_INFO("graphics pipeline created successfully");
    return {};
  }

  return FeErr{Error("failed to create graphics pipeline",
                     ErrorType::RendererVulkanError)};
}

void PipelineManager::DestroyGraphicsPipeline(Context &ctx,
                                              Pipeline *pPipeline) {
  if (pPipeline == nullptr) {
    return;
  }

  if (pPipeline->handle != nullptr) {
    vkDestroyPipeline(ctx.device.logicalDevice, pPipeline->handle,
                      ctx.pAllocator);
    pPipeline->handle = nullptr;
  }

  if (pPipeline->layout != nullptr) {
    vkDestroyPipelineLayout(ctx.device.logicalDevice, pPipeline->layout,
                            ctx.pAllocator);
    pPipeline->layout = nullptr;
  }
}

void PipelineManager::BindPipeline(CommandBuffer &cmdBuffer,
                              VkPipelineBindPoint bindPoint,
                              Pipeline &pipeline) {
  vkCmdBindPipeline(cmdBuffer.handle, bindPoint, pipeline.handle);
}

} // namespace flatearth::renderer::vulkan
