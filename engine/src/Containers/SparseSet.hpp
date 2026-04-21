#ifndef _FLATEARTH_ENGINE_CORE_SPARSE_SET_HPP
#define _FLATEARTH_ENGINE_CORE_SPARSE_SET_HPP

#include "Core/FeMemory.hpp"
#include "DArray.hpp"

#include <limits>

namespace flatearth::containers {

struct ISparseSetBase {
  virtual ~ISparseSetBase() = default;
  virtual void Remove(uint32 entityIndex) = 0;
  virtual uint32 *Entities() = 0;
  virtual uint64 Length() const = 0;
};

template <typename T>
class SparseSet : public ISparseSetBase {
  static_assert(std::is_trivially_copyable_v<T>,
                "SparseSet<T> is byte-wise; T must be trivially copyable");

  static constexpr uint32 scNull = std::numeric_limits<uint32>::max();

public:
  explicit SparseSet(memory::MemoryManager &memManager, uint32 maxEntities = 1024)
      : _dense(memManager), _denseEntities(memManager), _sparse(memManager),
        _cMaxEntities(maxEntities) {
    _sparse.Resize(maxEntities);
    for (uint32 i = 0; i < maxEntities; i++) {
      _sparse[i] = scNull;
    }
  }

  void Insert(uint32 entityIndex, const T &component) {
    if (entityIndex >= _cMaxEntities || Has(entityIndex)) {
      return;
    }

    _sparse[entityIndex] = static_cast<uint32>(_dense.Length());
    _dense.Push(component);
    _denseEntities.Push(entityIndex);
  }

  void Remove(uint32 entityIndex) override {
    if (!Has(entityIndex)) {
      return;
    }

    uint32 densePos = _sparse[entityIndex];
    uint32 lastPos = static_cast<uint32>(_dense.Length()) - 1;
    uint32 lastEntity = _denseEntities[lastPos];

    // swap target with last element to keep dense array packed
    _dense[densePos] = _dense[lastPos];
    _denseEntities[densePos] = lastEntity;
    _sparse[lastEntity] = densePos;

    _dense.Pop();
    _denseEntities.Pop();
    _sparse[entityIndex] = scNull;
  }

  bool Has(uint32 entityIndex) const {
    return entityIndex < _cMaxEntities && _sparse[entityIndex] != scNull;
  }

  void Clear() {
    for (uint32 i = 0; i < static_cast<uint32>(_denseEntities.Length()); i++) {
      _sparse[_denseEntities[i]] = scNull;
    }
    _dense.Clear();
    _denseEntities.Clear();
  }

  T &Get(uint32 entityIndex) { return _dense[_sparse[entityIndex]]; }

  const T &Get(uint32 entityIndex) const { return _dense[_sparse[entityIndex]]; }

  T *Data() { return _dense.Data(); }

  uint32 *Entities() override { return _denseEntities.Data(); }

  uint64 Length() const override { return _dense.Length(); }

  bool Empty() const { return _dense.Empty(); }

private:
  DArray<T> _dense;
  DArray<uint32> _denseEntities;
  DArray<uint32> _sparse;
  const uint32 _cMaxEntities;
};

} // namespace flatearth::containers

#endif // _FLATEARTH_ENGINE_CORE_SPARSE_SET_HPP
