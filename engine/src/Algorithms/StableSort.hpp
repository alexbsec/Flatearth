#ifndef _FLATEARTH_ENGINE_ALGORITHMS_STABLE_SORT_HPP
#define _FLATEARTH_ENGINE_ALGORITHMS_STABLE_SORT_HPP

#include "Containers/DArray.hpp"

namespace flatearth::algorithms {

namespace detail {

template <typename T, typename Comp>
static void Merge(T *arr,
                  T *scratch,
                  uint64 left,
                  uint64 mid,
                  uint64 right,
                  memory::MemoryManager &mem,
                  Comp comp) {
  mem.CopyMemory(scratch + left, arr + left, (right - left) * sizeof(T));

  uint64 i = left, j = mid, k = left;
  while (i < mid && j < right) {
    if (!comp(scratch[j], scratch[i])) {
      arr[k++] = scratch[i++];
    } else {
      arr[k++] = scratch[j++];
    }
  }

  while (i < mid) {
    arr[k++] = scratch[i++];
  }

  while (j < right) {
    arr[k++] = scratch[j++];
  }
}

template <typename T, typename Comp>
static void MergeSortImpl(
    T *arr, T *scratch, uint64 left, uint64 right, memory::MemoryManager &mem, Comp comp) {
  if (right - left <= 1) {
    return;
  }

  uint64 mid = left + (right - left) / 2;
  MergeSortImpl(arr, scratch, left, mid, mem, comp);
  MergeSortImpl(arr, scratch, mid, right, mem, comp);
  Merge(arr, scratch, left, mid, right, mem, comp);
}

} // namespace detail

template <typename T, typename Comp>
inline void StableSort(memory::MemoryManager &mem, containers::DArray<T> &arr, Comp comp) {
  uint64 n = arr.Length();
  if (n <= 1) {
    return;
  }

  T *scratch = FeCast<T>(mem.RawAlloc(n * sizeof(T), alignof(T), memory::Tag::DArray));
  detail::MergeSortImpl(arr.Data(), scratch, 0, n, mem, comp);
  mem.RawFree(reinterpret_cast<std::byte *>(scratch), n * sizeof(T), memory::Tag::DArray);
}

}; // namespace flatearth::algorithms

#endif // _FLATEARTH_ENGINE_CORE_ALGORITHMS_HPP
