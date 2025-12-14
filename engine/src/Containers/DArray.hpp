#ifndef _FLATEARTH_ENGINE_CORE_DARRAY_HPP
#define _FLATEARTH_ENGINE_CORE_DARRAY_HPP

#include "Defines.hpp"
#include "Error.hpp"
#include <expected>
namespace flatearth::containers {

template <typename T> class DArray {
public:
  static constexpr uint64 scDArrayDefaultSize = 1;
  static constexpr uint8 scDArrayResizeFactor = 2;
  static constexpr uint64 scDArrayFieldLength = 0;

public:
  DArray(uint64 stride = sizeof(T));
  DArray(uint64 capacity, uint64 stride = sizeof(T));
  ~DArray();

  uint64 Capacity() const;
  uint64 Length() const;
  uint64 Stride() const;

  void Reserve(uint64 size);

  void Push(const T &element);
  void Pop();

  std::expected<void, Error> InsertAt(const T &element, uint64 index);
  std::expected<void, Error> PopAt(uint64 index);
  void Clear();

  T *Data() noexcept;
  const T *Data() const noexcept;

  T &operator[](uint64 index);
  const T &operator[](uint64 index) const;

  bool Empty() const;

private:
  T *AddressOf(uint64 index) noexcept;
  const T *AddressOf(uint64 index) const noexcept;

private:
  void *_array;
  uint64 _capacity, _length, _stride;
};

template <typename T>
DArray<T>::DArray(uint64 stride)
    : _capacity(scDArrayDefaultSize), _length(0), _stride(stride) {
  _array = ::operator new(_capacity * _stride);  
}

template <typename T>
DArray<T>::DArray(uint64 capacity, uint64 stride)
    : _capacity(capacity), _length(0), _stride(stride) {
  _array = ::operator new(_capacity * _stride);  
}

template <typename T>
DArray<T>::~DArray() {
  ::operator delete(_array);
  _array = nullptr;
}

template <typename T> uint64 DArray<T>::Capacity() const {
  return _capacity; 
}


template <typename T> uint64 DArray<T>::Length() const {
  return _length; 
}

template <typename T> uint64 DArray<T>::Stride() const {
  return _stride; 
}

template <typename T> void DArray<T>::Reserve(uint64 size) {
  if (size <= _capacity) {
    return;
  }

  _capacity = size;

  uint64 headerSize = scDArrayFieldLength * sizeof(uint64);
  uint64 arraySize = _capacity * _stride;
  uint64 totalSize = headerSize + arraySize;

  T *element = ::operator new(totalSize);
}


}

#endif // _FLATEARTH_ENGINE_CORE_DARRAY_HPP
