#include "ObjectShader.hpp"
#include "Math/Vector3D.hpp"
#include "Renderer/Vulkan/VulkanTypes.hpp"
#include "Renderer/Vulkan/VulkanUtils.hpp"
#include <vulkan/vulkan_core.h>

namespace flatearth::renderer::vulkan::shaders {

VulkanShader::VulkanShader(memory::MemoryManager &memManager)
    : _memoryManager(memManager) {}

VulkanShader::~VulkanShader() {
  FLOG_INFO("vulkan shader successfully destroyed");
}

FeExpect<bool, Error>
VulkanShader::CreateObjectShader(Context &ctx, ObjectShader *pObjShader) {
  if (pObjShader == nullptr) {
    FLOG_ERROR("cannot create shader object on nullptr object shader");
    return FeErr{
        Error("object shader is nullptr", ErrorType::NullptrException)};
  }

  std::array<string, cObjectShaderStageCount> cStageTypeStrs{
      "vert",
      "frag",
  };

  constexpr std::array<VkShaderStageFlagBits, cObjectShaderStageCount>
      cStageTypes{
          VK_SHADER_STAGE_VERTEX_BIT,
          VK_SHADER_STAGE_FRAGMENT_BIT,
      };

  const string cBuiltinShaderName = "Builtin.ObjectShader";
  for (uint32 i = 0; i < cObjectShaderStageCount; i++) {
    auto createRes =
        CreateShaderModule(ctx, cBuiltinShaderName, cStageTypeStrs[i],
                           cStageTypes[i], i, pObjShader->shaderStages.data());
    if (!createRes.has_value()) {
      FLOG_ERROR("failed to create shader module at index {}", i);
      return FeErr{createRes.error()};
    }
    FLOG_INFO("{} object shader created", cStageTypeStrs[i]);
  }

  VkViewport viewport;
  viewport.x = 0.0f;
  viewport.y = static_cast<float32>(ctx.framebufferHeight);
  viewport.width = static_cast<float32>(ctx.framebufferWidth);
  viewport.height = viewport.y;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor;
  scissor.offset.x = scissor.offset.y = 0;
  scissor.extent.width = ctx.framebufferWidth;
  scissor.extent.height = ctx.framebufferHeight;

  uint32 offset = 0;
  const int32 attributeCount = 1;
  std::array<VkVertexInputAttributeDescription, attributeCount> attrDescription;
  std::array<VkFormat, attributeCount> formats = {VK_FORMAT_R32G32B32_SFLOAT};
  std::array<uint32, attributeCount> sizes = {sizeof(math::Vec3D)};

  for (uint32 i = 0; i < attributeCount; i++) {
    attrDescription[i].binding = 0;
    attrDescription[i].location = i;
    attrDescription[i].format = formats[i];
    attrDescription[i].offset = offset;
    offset += sizes[i];
  }

  // Stages
  std::array<VkPipelineShaderStageCreateInfo, cObjectShaderStageCount>
      stageInfos;
  _memoryManager.FZeroMemory(stageInfos.data(), sizeof(stageInfos));
  for (uint32 i = 0; i < cObjectShaderStageCount; i++) {
    stageInfos[i].sType =
        pObjShader->shaderStages[i].shaderStageCreateInfo.sType;
    stageInfos[i] = pObjShader->shaderStages[i].shaderStageCreateInfo;
  }

  auto pipelineRes = _pipelineManager.CreateGraphicsPipeline(
      ctx, &ctx.mainRenderpass, attributeCount, attrDescription.data(), 0, 0,
      cObjectShaderStageCount, stageInfos.data(), viewport, scissor, FeFalse,
      &pObjShader->pipeline);
  if (!pipelineRes.has_value()) {
    FLOG_ERROR("failed to create graphics pipeline");
    return FeErr{pipelineRes.error()};
  }

  return FeTrue;
}

void VulkanShader::DestroyObjectShader(Context &ctx, ObjectShader *pObjShader) {
  if (pObjShader == nullptr) {
    return;
  }

  _pipelineManager.DestroyGraphicsPipeline(ctx, &pObjShader->pipeline);
  for (uint32 i = 0; i < cObjectShaderStageCount; i++) {
    vkDestroyShaderModule(ctx.device.logicalDevice,
                          pObjShader->shaderStages[i].handle, ctx.pAllocator);
    pObjShader->shaderStages[i].handle = nullptr;
  }
}

void VulkanShader::UseShader(Context &ctx, ObjectShader &objShader) {
  // Placeholder implementation
  FLOG_INFO("using object shader");
}

} // namespace flatearth::renderer::vulkan::shaders
