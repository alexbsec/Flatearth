#ifndef _FLATEARTH_ENGINE_CORE_LINKED_LIST_HPP
#define _FLATEARTH_ENGINE_CORE_LINKED_LIST_HPP

#include "Defines.hpp"
namespace flatearth::containers {

template <typename T>
struct Node {
  T data;
  FePtr<Node<T>> pNext;
  Node(T data) : data(std::move(data)), pNext(nullptr) {}
};

} // namespace flatearth::containers

#endif // _FLATEARTH_ENGINE_CORE_LINKED_LIST_HPP
