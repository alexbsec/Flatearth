#ifndef _FLATEARTH_ENGINE_ECS_SCHEDULER_SYSTEM_SCHEDULER_HPP
#define _FLATEARTH_ENGINE_ECS_SCHEDULER_SYSTEM_SCHEDULER_HPP

#include "Containers/DArray.hpp"
#include "Core/FeMemory.hpp"
#include "Core/Logger.hpp"
#include "ECS/Scheduler/SystemPool.hpp"
#include "ECS/SystemInterface.hpp"

#include <utility>

namespace flatearth::ecs {

struct SystemNode {
  FePtr<ISystem> pSystem{nullptr};
  uint32 typeId{0};
  containers::DArray<uint32> before;
  containers::DArray<uint32> after;

  explicit SystemNode(memory::MemoryManager &memManager) : before(memManager), after(memManager) {}
};

class SystemBuilder {
public:
  explicit SystemBuilder(SystemNode &root) : _node(root) {}

  template <typename T>
  SystemBuilder &Before() {
    _node.before.Push(SystemTypeId<T>::Value());
    return *this;
  }

  template <typename T>
  SystemBuilder &After() {
    _node.after.Push(SystemTypeId<T>::Value());
    return *this;
  }

private:
  SystemNode &_node;
};

class SystemScheduler {
public:
  template <typename T, typename... Args>
  FeExpect<SystemBuilder, Error> Register(Args &&...args) {
    if (_built) {
      return FeErr{Error("cannot register any new systems on built SystemScheduler",
                         ErrorType::SystemError)};
    }

    uint32 typeId = SystemTypeId<T>::Value();
    FePtr<SystemNode> pNode =
        _memoryManager.Allocate<SystemNode>(memory::Tag::Entity, _memoryManager);
    pNode->typeId = typeId;
    pNode->pSystem =
        _memoryManager.Allocate<ISystem, T>(memory::Tag::Entity, std::forward<Args>(args)...);
    SystemBuilder builder{*pNode};
    auto res = _nodesMap.Insert(typeId, std::move(pNode))
                   .or_error("failed to insert system node into hashmap");
    if (res.errored()) {
      return FeErr{res.error()};
    }

    if (!res.value()) {
      FLOG_ERROR("insertion seemed to have failed with unknown reason");
    }

    return builder;
  }

  template <typename T>
  FeExpect<bool, Error> Unregister() {
    if (_built) {
      return FeErr{
          Error("cannot unregister systems on a built SystemScheduler. Call Reset() before.",
                ErrorType::SystemError)};
    }

    uint32 typeId = SystemTypeId<T>::Value();
    return _nodesMap.Erase(typeId);
  }

public:
  explicit SystemScheduler(memory::MemoryManager &memManager);
  FeExpect<void, Error> Build();
  FeExpect<void, Error> Rebuild(bool pruned = FeFalse);
  void Reset();
  void Prune();

  void BootSystems(Registry &registry);
  void Update(Registry &registry, float32 deltaTime);

private:
  memory::MemoryManager &_memoryManager;
  containers::HashMap<uint32, FePtr<SystemNode>> _nodesMap;
  containers::DArray<ISystem *> _sortedSystems;
  bool _built{FeFalse};
};

} // namespace flatearth::ecs

#endif // _FLATEARTH_ENGINE_ECS_SCHEDULER_SYSTEM_SCHEDULER_HPP
