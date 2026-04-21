#ifndef _FLATEARTH_ENGINE_CORE_HASH_SET_HPP
#define _FLATEARTH_ENGINE_CORE_HASH_SET_HPP

#include "Containers/LinkedList.hpp"
#include "Core/FeMemory.hpp"
#include "Error.hpp"

#include <cassert>

namespace flatearth::containers {

template <typename T>
struct Hash {
  uint64 operator()(const T &val) const { return std::hash<T>{}(val); }
};

template <typename T>
struct HashEqual {
  bool operator()(const T &a, const T &b) const { return a == b; }
};

template <typename T, typename H = Hash<T>, typename Eq = HashEqual<T>>
class HashSet {
public:
  explicit HashSet(memory::MemoryManager &memManager, uint64 bucketCount = 64)
      : _memoryManager(memManager), _bucketCount(bucketCount) {
    assert(bucketCount > 0);
    _ppBuckets = FeCast<FePtr<Node<T>>>(_memoryManager.RawAlloc(
        sizeof(FePtr<Node<T>>) * _bucketCount, alignof(FePtr<Node<T>>), memory::Tag::HashSet));

    for (uint64 i = 0; i < _bucketCount; i++) {
      new (&_ppBuckets[i]) FePtr<Node<T>>();
    }
  }

  ~HashSet() {
    if (_ppBuckets == nullptr) return;

    for (uint64 i = 0; i < _bucketCount; i++) {
      _ppBuckets[i].~FePtr<Node<T>>();
    }

    _memoryManager.RawFree(
        _ppBuckets, sizeof(FePtr<Node<T>>) * _bucketCount, memory::Tag::HashSet);
  }

  inline FeExpect<bool, Error> Insert(const T &value) {
    H hashFn{};
    Eq eqFn{};

    const uint64 index = hashFn(value) % _bucketCount;
    FePtr<Node<T>> &head = _ppBuckets[index];

    for (Node<T> *node = head.get(); node != nullptr; node = node->pNext.get()) {
      if (eqFn(node->data, value)) {
        // already exists
        return FeFalse;
      }
    }

    FePtr<Node<T>> newNode = _memoryManager.Allocate<Node<T>>(memory::Tag::HashSet, value);

    if (newNode == nullptr) {
      return FeErr{Error("allocation of Node<T> failed", ErrorType::NullptrException)};
    }

    newNode->pNext = std::move(head);
    head = std::move(newNode);
    _size++;
    return FeTrue;
  }

  inline bool Remove(const T &value) {
    H hashFn{};
    Eq eqFn{};

    const uint64 index = hashFn(value) % _bucketCount;
    FePtr<Node<T>> *owner = &_ppBuckets[index];

    while (owner->get() != nullptr) {
      Node<T> *node = owner->get();

      if (eqFn(node->data, value)) {
        *owner = std::move(node->pNext);
        _size--;
        return FeTrue;
      }

      owner = &node->pNext;
    }

    return FeFalse;
  }

  inline bool Has(const T &value) const {
    H hashFn{};
    Eq eqFn{};

    const uint64 index = hashFn(value) % _bucketCount;

    for (Node<T> *node = _ppBuckets[index].get(); node != nullptr; node = node->pNext.get()) {
      if (eqFn(node->data, value)) {
        return FeTrue;
      }
    }

    return FeFalse;
  }

  template <typename Fn>
  void ForEach(Fn &&fn) {
    for (uint64 i = 0; i < _bucketCount; i++) {
      for (Node<T> *node = _ppBuckets[i].get(); node != nullptr; node = node->pNext.get()) {
        fn(node->data);
      }
    }
  }

  inline uint64 Size() const { return _size; }

  inline bool Empty() const { return _size == 0; }

private:
  FePtr<Node<T>> *_ppBuckets{nullptr};
  memory::MemoryManager &_memoryManager;
  uint64 _bucketCount;
  uint64 _size{0};
};

} // namespace flatearth::containers

#endif // _FLATEARTH_ENGINE_CORE_HASH_SET_HPP
