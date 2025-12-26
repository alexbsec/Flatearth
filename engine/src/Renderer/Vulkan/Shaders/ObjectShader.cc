#include "ObjectShader.hpp"

namespace flatearth::renderer::vulkan::shaders {

VulkanShader::VulkanShader(memory::MemoryManager &memManager)
    : _memoryManager(memManager) {}

VulkanShader::~VulkanShader() {
  FLOG_INFO("vulkan shader successfully destroyed");
}

FeExpect<bool, Error> VulkanShader::CreateObjectShader(
    Context &ctx, ObjectShader *pObjShader) {
  // Placeholder implementation
  FLOG_INFO("object shader created");
  return FeTrue;
}

FeExpect<void, Error> VulkanShader::DestroyObjectShader(
    Context &ctx, ObjectShader *pObjShader) {
  // Placeholder implementation
  FLOG_INFO("object shader destroyed");
  return {};
}

void VulkanShader::UseShader(Context &ctx, ObjectShader &objShader) {
  // Placeholder implementation
  FLOG_INFO("using object shader");
}

}
