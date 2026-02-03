#ifndef _FLATEARTH_ENGINE_CORE_QUEUE_HPP
#define _FLATEARTH_ENGINE_CORE_QUEUE_HPP

#include "Containers/LinkedList.hpp"
#include "Core/FeMemory.hpp"
#include "Error.hpp"

namespace flatearth::containers {

template <typename T>
class Queue {
public:
  explicit Queue(memory::MemoryManager& memManager) : _memoryManager(memManager) {}

  inline FeExpect<void, Error> Put(const T& element) {
    FePtr<Node<T>> node = _memoryManager.Allocate<Node<T>>(memory::Tag::Queue, element);
    if (node == nullptr) {
      return FeErr{Error("allocation for new Node<T> failed", ErrorType::NullptrException)};
    }

    _size++;
    if (_pFront == nullptr) {
      _pFront = std::move(node);
      _pBack = _pFront.get();
      return {};
    }

    _pBack->pNext = std::move(node);
    _pBack = _pBack->pNext.get();
    return {};
  }

  inline void Pop() noexcept {
    if (_pFront == nullptr) {
      return;
    }

    _pFront = std::move(_pFront->pNext);
    if (_pFront == nullptr) {
      _pBack = nullptr;
    }

    _size--;
  }

  inline const T* Front() const noexcept {
    if (_pFront == nullptr) {
      return nullptr;
    }

    return &_pFront->data;
  }

  inline const T* Back() const noexcept {
    if (_pBack == nullptr) {
      return nullptr;
    }

    return &_pBack->data;
  }

  inline bool Empty() const noexcept { return _pFront == nullptr; }

private:
  FePtr<Node<T>> _pFront{nullptr};
  Node<T>* _pBack{nullptr};
  uint64 _size{0};
  memory::MemoryManager& _memoryManager;
};

} // namespace flatearth::containers

#endif // _FLATEARTH_ENGINE_CORE_QUEUE_HPP
