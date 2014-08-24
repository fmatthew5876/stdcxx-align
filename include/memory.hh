#ifndef _MEMORY_HH_
#define _MEMORY_HH_

#include <cstdio>
#include <memory>
#include <type_traits>

namespace std {

template <typename T>
  constexpr auto is_aligned(T x, size_t a) noexcept
  -> typename std::enable_if<std::is_integral<T>::value, bool>::type {
    return (x & (a-1)) == 0;
  }

template <typename T>
  constexpr auto align_up(T x, size_t a) noexcept
  -> typename std::enable_if<std::is_integral<T>::value, T>::type {
    return (x + (a-1)) & -a;
  }

template <typename T>
  constexpr auto align_down(T x, size_t a) noexcept
  -> typename std::enable_if<std::is_integral<T>::value, T>::type {
    return x & (-a);
  }


  template <typename T>
  inline auto is_aligned(T p, size_t a)
    -> typename std::enable_if<std::is_pointer<T>::value, bool>::type {
    return is_aligned(reinterpret_cast<uintptr_t>(p), a);
  }

  inline bool is_aligned(nullptr_t, size_t) {
    return true;
  }

  template <typename T>
    inline auto align_up(T p, size_t a)
    -> typename std::enable_if<std::is_pointer<T>::value, T>::type {
      return reinterpret_cast<T>(align_up(reinterpret_cast<uintptr_t>(p), a));
    }

  inline nullptr_t align_up(nullptr_t, size_t) { return nullptr; }

  template <typename T>
    inline auto align_down(T p, size_t a)
    -> typename std::enable_if<std::is_pointer<T>::value, T>::type {
      return reinterpret_cast<T>(align_down(reinterpret_cast<uintptr_t>(p), a));
    }

  inline nullptr_t align_down(nullptr_t, size_t) { return nullptr; }

  template <typename T, typename U>
    inline auto align_up_cast(U* p, size_t a=alignof(typename std::remove_pointer<T>::type))
    -> typename std::enable_if<std::is_pointer<T>::value, T>::type {
      return reinterpret_cast<T>(align_up(p, a));
    }

  template <typename T>
    inline auto align_up_cast(nullptr_t p, size_t a=1)
    -> typename std::enable_if<std::is_pointer<T>::value, T>::type {
      (void)a;
      nullptr;
    }

  template <typename T, typename U>
    inline auto align_down_cast(U* p, size_t a=alignof(typename std::remove_pointer<T>::type))
    -> typename std::enable_if<std::is_pointer<T>::value, T>::type {
      return reinterpret_cast<T>(align_down(p, a));
    }

  template <typename T>
    inline auto align_down_cast(nullptr_t p, size_t a=1)
    -> typename std::enable_if<std::is_pointer<T>::value, T>::type {
      (void)a;
      nullptr;
    }


} //namespace std

#endif
