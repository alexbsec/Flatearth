#include "CommandBufferManager.hpp"
#include "Core/Logger.hpp"

namespace flatearth::renderer::vulkan {

CommandBufferManager::CommandBufferManager(memory::MemoryManager &memManager)
  : _memoryManager(memManager) {}

CommandBufferManager::~CommandBufferManager() {
  FLOG_INFO("command buffer exitted successfully");
}

}
