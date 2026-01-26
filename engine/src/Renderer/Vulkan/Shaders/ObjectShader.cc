#include "ObjectShader.hpp"
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

  return FeTrue;
}

FeExpect<void, Error>
VulkanShader::DestroyObjectShader(Context &ctx, ObjectShader *pObjShader) {
  // Placeholder implementation
  FLOG_INFO("object shader destroyed");
  return {};
}

void VulkanShader::UseShader(Context &ctx, ObjectShader &objShader) {
  // Placeholder implementation
  FLOG_INFO("using object shader");
}

} // namespace flatearth::renderer::vulkan::shaders
