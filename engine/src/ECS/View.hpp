#ifndef _FLATEARTH_ENGINE_ECS_VIEW_HPP
#define _FLATEARTH_ENGINE_ECS_VIEW_HPP

#include "Containers/SparseSet.hpp"
#include "ECS/ComponentPool.hpp"
#include "ECS/ECSTypes.hpp"

#include <limits>
#include <tuple>

namespace flatearth::ecs {

template <typename... Ts>
class View {
public:
  explicit View(containers::SparseSet<Ts> *...sets) : _sets(sets...) { _pDriver = FindSmallest(); }

  struct Iterator {
    View &_view;
    uint32 _index;

    bool operator!=(const Iterator &other) const { return _index != other._index; }

    Iterator &operator++() {
      ++_index;
      Advance();
      return *this;
    }

    std::tuple<EntityId, Ts &...> operator*() {
      EntityId id = _view._pDriver->Entities()[_index];
      return {id, _view.template Get<Ts>(id)...};
    }

    friend class View;

  private:
    void Advance() {
      while (_index < _view._pDriver->Length() &&
             !_view.HasAll(_view._pDriver->Entities()[_index])) {
        ++_index;
      }
    }
  };

  Iterator begin() {
    Iterator it{*this, 0};
    it.Advance();
    return it;
  }

  Iterator end() { return {*this, static_cast<uint32>(_pDriver->Length())}; }

  bool Empty() const { return _pDriver == nullptr || _pDriver->Length() == 0; }

private:
  std::tuple<containers::SparseSet<Ts> *...> _sets;
  ISparseSetBase *_pDriver{nullptr};

  template <typename T>
  containers::SparseSet<T> *GetSet() {
    return std::get<containers::SparseSet<T> *>(_sets);
  }

  template <typename T>
  T &Get(EntityId id) {
    return GetSet<T>()->Get(id);
  }

  bool HasAll(EntityId id) {
    return (... && std::get<containers::SparseSet<Ts> *>(_sets)->Has(id));
  }

  ISparseSetBase *FindSmallest() {
    ISparseSetBase *pSmallest = nullptr;
    uint64 minLen = std::numeric_limits<uint64>::max();
    std::apply(
        [&](auto *...sets) {
          ((sets->Length() < minLen ? (minLen = sets->Length(), pSmallest = sets) : nullptr), ...);
        },
        _sets);
    return pSmallest;
  }
};

} // namespace flatearth::ecs

#endif // _FLATEARTH_ENGINE_ECS_VIEW_HPP
