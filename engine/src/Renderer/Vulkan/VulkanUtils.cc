#include "VulkanUtils.hpp"

#include "BuiltinShaders.hpp"
#include "Core/Logger.hpp"
#include "Resources/ResourceTypes.hpp"
#include <vulkan/vulkan_core.h>

namespace flatearth::renderer::vulkan {

void EnsureGPUMatrixLayout(math::Mat4D &inProj, math::Mat4D &inView) {
  inProj = inProj.ToGPUMatrix();
  inView = inView.ToGPUMatrix();
}

FeExpect<bool, Error> CreateShaderModule(Context &ctx,
                                         const string &name,
                                         const string &typeStr,
                                         VkShaderStageFlagBits stageFlag,
                                         uint32 stageIndex,
                                         ShaderStage *pShaderStage) {
  const uint8_t *pCode = nullptr;
  size_t codeSize = 0;

  if (name == "Builtin.ObjectShader" && typeStr == "vert") {
    pCode = shaders::kObjectShaderVert;
    codeSize = shaders::kObjectShaderVertSize;
  } else if (name == "Builtin.ObjectShader" && typeStr == "frag") {
    pCode = shaders::kObjectShaderFrag;
    codeSize = shaders::kObjectShaderFragSize;
  } else {
    FLOG_ERROR("no embedded shader for {}.{}", name, typeStr);
    return FeErr{Error("unknown built-in shader", ErrorType::FileReadError)};
  }

  if (codeSize == 0 || (codeSize % 4) != 0) {
    FLOG_ERROR("invalid embedded SPIR-V size for {}.{}: {} bytes", name, typeStr, codeSize);
    return FeErr{Error("bad SPIR-V size", ErrorType::InvalidFileHandle)};
  }

  ctx.memoryManager.FZeroMemory(&pShaderStage[stageIndex].shaderModuleCreateInfo,
                                sizeof(VkShaderModuleCreateInfo));
  pShaderStage[stageIndex].shaderModuleCreateInfo.sType =
      VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  pShaderStage[stageIndex].shaderModuleCreateInfo.codeSize = codeSize;
  pShaderStage[stageIndex].shaderModuleCreateInfo.pCode =
      reinterpret_cast<const uint32_t *>(pCode);

  if (auto res = VkCheck(vkCreateShaderModule(ctx.device.logicalDevice,
                                              &pShaderStage[stageIndex].shaderModuleCreateInfo,
                                              ctx.pAllocator,
                                              &pShaderStage[stageIndex].handle));
      res.errored()) {
    FLOG_ERROR("failed to create shader module for {}.{}", name, typeStr);
    return FeErr{res.error()};
  }

  ctx.memoryManager.FZeroMemory(&pShaderStage[stageIndex].shaderStageCreateInfo,
                                sizeof(VkPipelineShaderStageCreateInfo));
  pShaderStage[stageIndex].shaderStageCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  pShaderStage[stageIndex].shaderStageCreateInfo.stage = stageFlag;
  pShaderStage[stageIndex].shaderStageCreateInfo.module = pShaderStage[stageIndex].handle;
  pShaderStage[stageIndex].shaderStageCreateInfo.pName = "main";

  pShaderStage[stageIndex].shaderModuleCreateInfo.pCode = nullptr;
  pShaderStage[stageIndex].shaderModuleCreateInfo.codeSize = 0;
  return FeTrue;
}

VkSamplerCreateInfo SamplerInfoByFilter(resources::TextureFilter filter) {
  VkSamplerCreateInfo samplerInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable = VK_TRUE;
  samplerInfo.maxAnisotropy = 16;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;

  if (filter == resources::TextureFilter::Nearest) {
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  }

  return samplerInfo;
}

} // namespace flatearth::renderer::vulkan
