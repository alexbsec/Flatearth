#include "VulkanUtils.hpp"
#include "Core/Logger.hpp"
#include "Platform/Filesystem.hpp"

namespace flatearth::renderer::vulkan {

void ShaderModuleCleanup(Context &ctx,
                         platform::FileHandle fileHandle,
                         uint32 *pWords,
                         uint64 fileSize) {
  if (pWords != nullptr) {
    ctx.memoryManager.RawFree(pWords, fileSize, memory::Tag::Renderer);
  }

  auto closeRes = ctx.filesystem.CloseFile(fileHandle);
  if (!closeRes.has_value()) {
    FLOG_FATAL("failed to close file in cleanup stage");
  }
}


FeExpect<bool, Error> CreateShaderModule(Context &ctx, const string &name,
                                         const string &typeStr,
                                         VkShaderStageFlagBits stageFlag,
                                         uint32 stageIndex,
                                         ShaderStage *pShaderStage) {
  const string cFileName =
      std::format("bin/assets/shaders/{}.{}.spv", name, typeStr);

  ctx.memoryManager.FZeroMemory(
      &pShaderStage[stageIndex].shaderModuleCreateInfo,
      sizeof(VkShaderModuleCreateInfo));
  pShaderStage[stageIndex].shaderModuleCreateInfo.sType =
      VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

  auto openRes = ctx.filesystem.OpenFile(cFileName, platform::FileMode::Read);
  if (!openRes.has_value()) {
    FLOG_ERROR("failed to open file at path {}: {}", cFileName,
               openRes.error().message);
    return FeErr{openRes.error()};
  }
  platform::FileHandle fileHandle = openRes.value();

  uint64 fileSize = ctx.filesystem.SizeOfFile(cFileName);
  if (fileSize == 0 || (fileSize % 4) != 0) {
    FLOG_ERROR("invalid SPIR-V size for path {}. Size: {}", cFileName,
               fileSize);
    ShaderModuleCleanup(ctx, fileHandle, nullptr, fileSize);
    return FeErr{Error("bad SPIR-V file size", ErrorType::InvalidFileHandle)};
  }

  uint32 wordCount = static_cast<uint32>(fileSize / 4);
  uint32 *words = static_cast<uint32 *>(ctx.memoryManager.RawAlloc(
      fileSize, alignof(uint32), memory::Tag::Renderer));
  if (words == nullptr) {
    FLOG_ERROR("failed to allocate words for reading");
    ShaderModuleCleanup(ctx, fileHandle, nullptr, fileSize);
    return FeErr{Error("allocation failed", ErrorType::NullptrException)};
  }

  std::span<std::byte> bytes{
      reinterpret_cast<std::byte *>(words),
      fileSize,
  };

  auto readRes = ctx.filesystem.ReadFromFile(fileHandle, bytes);
  if (!readRes.has_value()) {
    FLOG_ERROR("failed to read file at path {}: {}", cFileName,
               readRes.error().message);
    ShaderModuleCleanup(ctx, fileHandle, words, fileSize);
    return FeErr{readRes.error()};
  }

  pShaderStage[stageIndex].shaderModuleCreateInfo.codeSize = fileSize;
  pShaderStage[stageIndex].shaderModuleCreateInfo.pCode = words;

  if (auto res = VkCheck(vkCreateShaderModule(
          ctx.device.logicalDevice,
          &pShaderStage[stageIndex].shaderModuleCreateInfo, ctx.pAllocator,
          &pShaderStage[stageIndex].handle));
      !res.has_value()) {
    FLOG_ERROR("failed to create shader module");
    ShaderModuleCleanup(ctx, fileHandle, words, fileSize);
    return FeErr{res.error()};
  }

  ctx.memoryManager.FZeroMemory(&pShaderStage[stageIndex].shaderStageCreateInfo,
                                sizeof(VkPipelineShaderStageCreateInfo));
  pShaderStage[stageIndex].shaderStageCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  pShaderStage[stageIndex].shaderStageCreateInfo.stage = stageFlag;
  pShaderStage[stageIndex].shaderStageCreateInfo.module =
      pShaderStage[stageIndex].handle;
  pShaderStage[stageIndex].shaderStageCreateInfo.pName = "main";

  ShaderModuleCleanup(ctx, fileHandle, words, fileSize);
  pShaderStage[stageIndex].shaderModuleCreateInfo.pCode = nullptr;
  pShaderStage[stageIndex].shaderModuleCreateInfo.codeSize = 0;

  return FeTrue;
}

} // namespace flatearth::renderer::vulkan
