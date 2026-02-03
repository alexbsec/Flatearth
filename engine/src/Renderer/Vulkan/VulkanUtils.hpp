#ifndef _FLATEARTH_ENGINE_RENDERER_VULKAN_UTILS_HPP
#define _FLATEARTH_ENGINE_RENDERER_VULKAN_UTILS_HPP

#include "Defines.hpp"
#include "Renderer/Vulkan/VulkanTypes.hpp"

namespace flatearth::renderer::vulkan {

void EnsureGPUMatrixLayout(math::Mat4D &inProj, math::Mat4D &inView);

FeExpect<bool, Error> CreateShaderModule(Context &ctx,
                                         const string &name,
                                         const string &typeStr,
                                         VkShaderStageFlagBits stageFlag,
                                         uint32 stageIndex,
                                         ShaderStage *pShaderStage);

}

#endif // _FLATEARTH_ENGINE_RENDERER_VULKAN_UTILS_HPP
