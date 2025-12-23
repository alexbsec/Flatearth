#ifndef _FLATEARTH_ENGINE_CORE_DARRAY_HPP
#define _FLATEARTH_ENGINE_CORE_DARRAY_HPP

#include "Core/FeMemory.hpp"
#include "Defines.hpp"
#include "Error.hpp"
#include <cstddef>
#include <cstring>
#include <expected>

namespace flatearth::containers {

struct DArrayRawDeleter {
  memory::MemoryManager *mem{nullptr};
  uint64 size{};
  memory::Tag tag{};

  void operator()(std::byte *ptr) const noexcept {
    if (ptr) {
      mem->RawFree(ptr, size, tag);
    }
  }
};

template <typename T> class DArray {
  static_assert(std::is_trivially_copyable_v<T>,
                "DArray<T> is byte-wise; T must be trivially copyable");

public:
  static constexpr uint64 scDArrayDefaultSize = 1;
  static constexpr uint8 scDArrayResizeFactor = 2;

public:
  explicit DArray(memory::MemoryManager &memManager, uint64 stride = sizeof(T))
      : _capacity(scDArrayDefaultSize), _length(0), _stride(stride),
        _bufferSize(0), _memoryManager(memManager) {
    InitDArray();
  }

  explicit DArray(memory::MemoryManager &memManager, uint64 capacity,
                  uint64 stride = sizeof(T))
      : _capacity(capacity), _length(0), _stride(stride), _bufferSize(0),
        _memoryManager(memManager) {
    InitDArray();
  }

  ~DArray() = default;

  uint64 Capacity() const { return _capacity; }
  uint64 Length() const { return _length; }
  uint64 Stride() const { return _stride; }

  void Reserve(uint64 size) {
    if (size <= _capacity || size < _length) {
      return;
    }

    uint64 newBufferSize = size * _stride;
    std::byte *newMem = static_cast<std::byte *>(_memoryManager.RawAlloc(
        newBufferSize, alignof(T), memory::Tag::DArray));

    // copy existing bytes (POD semantics)
    if (_array) {
      std::memcpy(newMem, _array.get(), _length * _stride);
    }

    // replace buffer + deleter (so size matches)
    _array = FeCustomDeleterPtr<std::byte[], DArrayRawDeleter>(
        newMem,
        DArrayRawDeleter{&_memoryManager, newBufferSize, memory::Tag::DArray});

    _capacity = size;
    _bufferSize = newBufferSize;
  }

  void Push(const T &element) {
    if (_length >= _capacity) {
      Reserve(_capacity * scDArrayResizeFactor);
    }

    std::memcpy(AddressOf(_length), &element, _stride);
    ++_length;
  }

  void Pop() {
    if (_length == 0)
      return;
    --_length;
  }

  FeExpect<void, Error> InsertAt(const T &element, uint64 index) {
    if (index > _length) {
      return FeErr{Error("InsertAt: index out of bounds")};
    }

    if (_length >= _capacity) {
      Reserve(_capacity * scDArrayResizeFactor);
    }

    std::memmove(AddressOf(index + 1), AddressOf(index),
                 (_length - index) * _stride);

    std::memcpy(AddressOf(index), &element, _stride);
    ++_length;
    return {};
  }

  FeExpect<void, Error> PopAt(uint64 index) {
    if (index >= _length) {
      return FeErr{Error("PopAt: index out of bounds")};
    }

    std::memmove(AddressOf(index), AddressOf(index + 1),
                 (_length - index - 1) * _stride);

    --_length;
    return {};
  }

  void Clear() { _length = 0; }

  T *Data() noexcept { return reinterpret_cast<T *>(_array.get()); }
  const T *Data() const noexcept {
    return reinterpret_cast<const T *>(_array.get());
  }

  T &operator[](uint64 index) { return *AddressOf(index); }
  const T &operator[](uint64 index) const { return *AddressOf(index); }

  bool Empty() const { return _length == 0; }

private:
  using RawPtr = FeCustomDeleterPtr<std::byte[], DArrayRawDeleter>;

  void InitDArray() noexcept {
    _bufferSize = _capacity * _stride;
    std::byte *mem = static_cast<std::byte *>(
        _memoryManager.RawAlloc(_bufferSize, alignof(T), memory::Tag::DArray));

    _array = RawPtr(mem, DArrayRawDeleter{&_memoryManager, _bufferSize,
                                          memory::Tag::DArray});
  }

  T *AddressOf(uint64 index) noexcept {
    return reinterpret_cast<T *>(_array.get() + index * _stride);
  }

  const T *AddressOf(uint64 index) const noexcept {
    return reinterpret_cast<const T *>(_array.get() + index * _stride);
  }

private:
  RawPtr _array;
  uint64 _capacity{0}, _length{0}, _stride{0}, _bufferSize{0};
  memory::MemoryManager &_memoryManager;
};

} // namespace flatearth::containers

#endif // _FLATEARTH_ENGINE_CORE_DARRAY_HPP
