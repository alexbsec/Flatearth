#include "SystemScheduler.hpp"

#include "Containers/Queue.hpp"
#include "Core/FeMemory.hpp"

namespace flatearth::ecs {

void BuildInDegree(const FePtr<SystemNode> &node, containers::HashMap<uint32, uint32> &inDegree);
void BuildEdges(const FePtr<SystemNode> &node,
                containers::HashMap<uint32, containers::DArray<uint32>> &edges,
                memory::MemoryManager &memManager);

SystemScheduler::SystemScheduler(memory::MemoryManager &memManager)
    : _memoryManager(memManager), _nodesMap(memManager), _sortedSystems(memManager) {
}

FeExpect<void, Error> SystemScheduler::Build() {
  using namespace containers;
  if (_built) {
    FLOG_INFO("SystemScheduler was already built");
    return {};
  }

  // initialize inDegrees hash map
  HashMap<uint32, uint32> inDegree(_memoryManager);
  _nodesMap.ForEach([&](uint32 typeId, const auto &) {
    auto res = inDegree.Insert(typeId, 0);
    if (!res.has_value()) {
      FLOG_WARN("failed to insert typeId {} into inDegree array. This initialization error may "
                "lead to crashes and UB. Err: {}",
                typeId,
                res.error().message);
    }
  });

  // build the adjacency list
  HashMap<uint32, DArray<uint32>> edges(_memoryManager);
  _nodesMap.ForEach([&](uint32, const FePtr<SystemNode> &node) {
    BuildInDegree(node, inDegree);
    BuildEdges(node, edges, _memoryManager);
  });

  // queue seeding
  Queue<uint32> queue(_memoryManager);
  inDegree.ForEach([&](uint32 typeId, const uint32 &degree) {
    if (degree != 0) {
      return;
    }

    auto res = queue.Put(typeId);
    if (!res.has_value()) {
      FLOG_WARN("could not insert typeId {} into queue. Err: {}", typeId, res.error().message);
    }
  });

  uint32 processed = 0;
  while (!queue.Empty()) {
    const uint32 *pTypeId = queue.Front();
    if (pTypeId == nullptr) {
      FLOG_WARN("failed to get const ptr reference of queue");
      queue.Pop();
      continue;
    }
    // Copy value before Pop() frees the node.
    const uint32 typeId = *pTypeId;
    queue.Pop();

    FePtr<SystemNode> *ppNode = _nodesMap.Retrieve(typeId);
    if (ppNode == nullptr) {
      FLOG_WARN("failed to get ptr to FePtr<SystemNode>: no reference found");
      continue;
    }
    ISystem *pRawSystem = ppNode->get()->pSystem.get();
    _sortedSystems.Push(pRawSystem);
    processed++;

    DArray<uint32> *pNeighbors = edges.Retrieve(typeId);
    if (pNeighbors == nullptr) {
      // no neighbors
      continue;
    }

    for (uint32 i = 0; i < pNeighbors->Length(); i++) {
      uint32 neighborId = (*pNeighbors)[i];
      uint32 *pId = inDegree.Retrieve(neighborId);
      if (pId == nullptr) {
        FLOG_WARN("failed to retrieve ptr to uint32 neighbor ids: no reference found");
        continue;
      }

      *pId -= 1;
      if (*pId > 0) {
        continue;
      }

      auto res = queue.Put(neighborId);
      if (!res.has_value()) {
        FLOG_ERROR("failed to put neighbor id in queue: {}", res.error().message);
      }
    }
  }

  if (processed != _nodesMap.Size()) {
    FLOG_ERROR("cycle detected");
    string out = std::format("the number of processed nodes '{}' is not equal _nodesMap.Size '{}'",
                             processed,
                             _nodesMap.Size());
    return FeErr{Error(out, ErrorType::LogicalError)};
  }

  _built = FeTrue;
  return {};
}

FeExpect<void, Error> SystemScheduler::Rebuild(bool pruned) {
  if (!_built) {
    return FeErr{Error("cannot rebuild a non-built SystemScheduler", ErrorType::SystemError)};
  }

  if (pruned) {
    Prune();
  } else {
    Reset();
  }
  return Build();
}

void SystemScheduler::Reset() {
  _built = FeFalse;
  _sortedSystems.Clear();
}

void SystemScheduler::Prune() {
  Reset();
  _nodesMap.Clear();
}

void SystemScheduler::Update(Registry &registry, float32 deltaTime) {
  if (!_built) {
    return;
  }

  for (uint64 i = 0; i < _sortedSystems.Length(); i++) {
    _sortedSystems[i]->Update(registry, deltaTime);
  }
}

void BuildInDegree(const FePtr<SystemNode> &node, containers::HashMap<uint32, uint32> &inDegree) {
  for (uint64 i = 0; i < node->after.Length(); i++) {
    uint32 *pCounter = inDegree.Retrieve(node->typeId);
    if (pCounter != nullptr) {
      *pCounter += 1;
    }
  }

  for (uint64 i = 0; i < node->before.Length(); i++) {
    uint32 typeId = node->before[i];
    uint32 *pCounter = inDegree.Retrieve(typeId);
    if (pCounter != nullptr) {
      *pCounter += 1;
    }
  }
}

void BuildEdges(const FePtr<SystemNode> &node,
                containers::HashMap<uint32, containers::DArray<uint32>> &edges,
                memory::MemoryManager &memManager) {
  for (uint64 i = 0; i < node->after.Length(); i++) {
    uint32 x = node->after[i];
    containers::DArray<uint32> *pList = edges.Retrieve(x);
    if (pList == nullptr) {
      auto res = edges.Insert(x, std::move(containers::DArray<uint32>(memManager)));
      if (!res.has_value()) {
        FLOG_ERROR("failed to insert a new DArray while building edges. Err: {}",
                   res.error().message);
        continue;
      }
      pList = edges.Retrieve(x);
    }
    pList->Push(node->typeId);
  }

  for (uint64 i = 0; i < node->before.Length(); i++) {
    uint32 x = node->before[i];
    containers::DArray<uint32> *pList = edges.Retrieve(node->typeId);
    if (pList == nullptr) {
      auto res = edges.Insert(node->typeId, std::move(containers::DArray<uint32>(memManager)));
      if (!res.has_value()) {
        FLOG_ERROR("failed to insert a new DArray while building edges. Err: {}",
                   res.error().message);
        continue;
      }
      pList = edges.Retrieve(node->typeId);
    }
    pList->Push(x);
  }
}

} // namespace flatearth::ecs
