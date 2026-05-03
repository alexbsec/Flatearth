#ifndef _FLATEARTH_ENGINE_CORE_HASH_MAP_HPP
#define _FLATEARTH_ENGINE_CORE_HASH_MAP_HPP

#include "Containers/HashSet.hpp"
#include "Containers/LinkedList.hpp"
#include "Core/FeMemory.hpp"
namespace flatearth::containers {

template <typename T, typename V>
struct HashMapEntry {
  T key;
  V value;
};

template <typename T, typename V, typename H = Hash<T>, typename Eq = HashEqual<T>>
class HashMap {
public:
  explicit HashMap(memory::MemoryManager &memManager, uint64 bucketCount = 64)
      : _memoryManager(memManager), _bucketCount(bucketCount) {
    assert(bucketCount > 0);

    _ppBuckets = FeCast<FePtr<Node<HashMapEntry<T, V>>>>(
        _memoryManager.RawAlloc(sizeof(FePtr<Node<HashMapEntry<T, V>>>) * _bucketCount,
                                alignof(FePtr<Node<HashMapEntry<T, V>>>),
                                memory::Tag::HashMap));

    for (uint64 i = 0; i < _bucketCount; i++) {
      new (&_ppBuckets[i]) FePtr<Node<HashMapEntry<T, V>>>();
    }
  }

  ~HashMap() {
    if (_ppBuckets == nullptr) {
      return;
    }

    using BucketPtr = FePtr<Node<HashMapEntry<T, V>>>;
    for (uint64 i = 0; i < _bucketCount; i++) {
      _ppBuckets[i].~BucketPtr();
    }

    _memoryManager.RawFree(
        _ppBuckets, sizeof(FePtr<Node<HashMapEntry<T, V>>>) * _bucketCount, memory::Tag::HashMap);
  }

  inline FeExpect<bool, Error> Insert(const T &key, const V &val) {
    H hashFn{};
    Eq eqFn{};

    const uint64 index = hashFn(key) % _bucketCount;
    FePtr<Node<HashMapEntry<T, V>>> &head = _ppBuckets[index];

    for (Node<HashMapEntry<T, V>> *node = head.get(); node != nullptr; node = node->pNext.get()) {
      if (eqFn(node->data.key, key)) {
        node->data.value = val;
        return FeTrue;
      }
    }

    HashMapEntry<T, V> entry{key, val};
    FePtr<Node<HashMapEntry<T, V>>> newNode =
        _memoryManager.Allocate<Node<HashMapEntry<T, V>>>(memory::Tag::HashMap, entry);

    if (newNode == nullptr) {
      return FeErr{Error("allocation of HashMap node failed", ErrorType::NullptrException)};
    }

    newNode->pNext = std::move(head);
    head = std::move(newNode);
    _size++;
    return FeTrue;
  }

  inline FeExpect<bool, Error> Insert(const T &key, V &&val) {
    H hashFn{};
    Eq eqFn{};

    const uint64 index = hashFn(key) % _bucketCount;
    FePtr<Node<HashMapEntry<T, V>>> &head = _ppBuckets[index];

    for (Node<HashMapEntry<T, V>> *node = head.get(); node != nullptr; node = node->pNext.get()) {
      if (eqFn(node->data.key, key)) {
        node->data.value = std::move(val);
        return FeTrue;
      }
    }

    HashMapEntry<T, V> entry{key, std::move(val)};
    FePtr<Node<HashMapEntry<T, V>>> newNode =
        _memoryManager.Allocate<Node<HashMapEntry<T, V>>>(memory::Tag::HashMap, std::move(entry));

    if (newNode == nullptr) {
      return FeErr{Error("allocation of HashMap node failed", ErrorType::NullptrException)};
    }

    newNode->pNext = std::move(head);
    head = std::move(newNode);
    _size++;
    return FeTrue;
  }

  inline V *Retrieve(const T &key) {
    H hashFn{};
    Eq eqFn{};

    const uint64 index = hashFn(key) % _bucketCount;

    for (Node<HashMapEntry<T, V>> *node = _ppBuckets[index].get(); node != nullptr;
         node = node->pNext.get()) {
      if (eqFn(node->data.key, key)) {
        return &node->data.value;
      }
    }

    return nullptr;
  }

  inline const V *Retrieve(const T &key) const {
    return const_cast<HashMap *>(this)->Retrieve(key);
  }

  inline bool Has(const T &key) const { return Retrieve(key) != nullptr; }

  inline bool Erase(const T &key) {
    H hashFn{};
    Eq eqFn{};

    const uint64 index = hashFn(key) % _bucketCount;
    FePtr<Node<HashMapEntry<T, V>>> *ppOwner = &_ppBuckets[index];
    while (ppOwner->get() != nullptr) {
      Node<HashMapEntry<T, V>> *node = ppOwner->get();

      if (eqFn(node->data.key, key)) {
        FePtr<Node<HashMapEntry<T, V>>> toDelete = std::move(*ppOwner); // take ownership
        *ppOwner = std::move(toDelete->pNext);                          // splice successor in
        _size--;
        return FeTrue; // toDelete frees node
      }

      ppOwner = &node->pNext;
    }

    return FeFalse;
  }

  template <typename Fn>
  void ForEach(Fn &&fn) {
    for (uint64 i = 0; i < _bucketCount; i++) {
      for (Node<HashMapEntry<T, V>> *node = _ppBuckets[i].get(); node != nullptr;
           node = node->pNext.get()) {
        fn(node->data.key, node->data.value);
      }
    }
  }

  void Clear() {
    for (uint64 i = 0; i < _bucketCount; i++) {
      _ppBuckets[i].reset();
    }
    _size = 0;
  }

  inline uint64 Size() const { return _size; }
  inline bool Empty() const { return _size == 0; }

private:
  FePtr<Node<HashMapEntry<T, V>>> *_ppBuckets{nullptr};
  memory::MemoryManager &_memoryManager;
  uint64 _bucketCount;
  uint64 _size{0};
};

} // namespace flatearth::containers

#endif // _FLATEARTH_ENGINE_CORE_HASH_MAP_HPP
