#ifndef _MEMORY_HH_
#define _MEMORY_HH_

#include <memory>

namespace std {
template <typename T>
  constexpr bool is_aligned(T x, size_t align) noexcept {
    return (x & (align-1)) == 0;
  }

template <typename T>
  constexpr T align_up(T x, size_t align) noexcept {
    return (x + (align-1)) & -a;
  }

template <typename T>
  constexpr T align_down(T x, size_t align) noexcept {
    return x & (-align);
  }

#endif
