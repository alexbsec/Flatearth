#include "ShaderRegistry.hpp"

#include "Renderer/Vulkan/VulkanTypes.hpp"

namespace flatearth::renderer {

ShaderRegistry::ShaderRegistry(memory::MemoryManager &mm)
    : _shadersMap(mm), _handlesMap(mm), _memoryManager(mm) {
}

FeExpect<ShaderHandle, Error> ShaderRegistry::NewShader(stringv name,
                                                        ShaderInterpreter interpreter) {
  const string nameStr = string(name);
  ShaderHandle *pHandle = _handlesMap.Retrieve(nameStr);
  if (pHandle != nullptr) {
    return *pHandle;
  }

  ShaderHandle thisHandle = _nextHandle++;
  auto insertRes =
      _handlesMap.Insert(nameStr, thisHandle).or_error("failed to insert new handle into HashMap");
  if (insertRes.errored()) {
    return FeErr{insertRes.error()};
  }

  FePtr<Shader> newShader;
  switch (interpreter) {
    case ShaderInterpreter::Vulkan: {
      newShader = _memoryManager.Allocate<Shader, vulkan::ObjectShader>(memory::Tag::Renderer,
                                                                        _memoryManager);
      break;
    }
    default:
      return FeErr{Error("Backend not implemented", ErrorType::NotImplemented)};
  }

  insertRes = _shadersMap.Insert(thisHandle, std::move(newShader))
                  .or_error("failed to insert constructed shader into HashMap");
  if (insertRes.errored()) {
    return FeErr{insertRes.error()};
  }

  return thisHandle;
}

Shader *ShaderRegistry::RetrieveShader(stringv name) {
  const string nameStr = string(name);
  const ShaderHandle *pHandle = _handlesMap.Retrieve(nameStr);
  if (pHandle == nullptr) {
    return nullptr;
  }

  return RetrieveShader(*pHandle);
}

const Shader *ShaderRegistry::RetrieveShader(stringv name) const {
  return const_cast<ShaderRegistry *>(this)->RetrieveShader(name);
}

Shader *ShaderRegistry::RetrieveShader(ShaderHandle handle) {
  FePtr<Shader> *ppShader = _shadersMap.Retrieve(handle);
  if (ppShader == nullptr) {
    return nullptr;
  }

  return ppShader->get();
}

const Shader *ShaderRegistry::RetrieveShader(ShaderHandle handle) const {
  return const_cast<ShaderRegistry *>(this)->RetrieveShader(handle);
}

void ShaderRegistry::DestroyShader(stringv name) {
  const string nameStr{name};

  ShaderHandle *pHandle = _handlesMap.Retrieve(nameStr);
  if (pHandle == nullptr) {
    return;
  }

  if (!_shadersMap.Has(*pHandle)) {
    FLOG_WARN("handlesMap has '{}' shader but no Shader is created internally", name);
  } else {
    _shadersMap.Erase(*pHandle);
  }

  _handlesMap.Erase(nameStr);
}

} // namespace flatearth::renderer
