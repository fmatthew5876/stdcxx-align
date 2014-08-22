#ifndef _MEMORY_HH_
#define _MEMORY_HH_

#include <cstdio>
#include <memory>
#include <type_traits>

namespace std {

template <typename T, typename V> using __carry_cv =
  conditional<is_const<T>::value,
  conditional<is_volatile<T>::value,const volatile V,const V>,
  conditional<is_volatile<T>::value,volatile V, V>>;

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

  inline bool is_aligned(void* p, size_t a) {
    return is_aligned(reinterpret_cast<uintptr_t>(p), a);
  }
  inline bool is_aligned(const void* p, size_t a) {
    return is_aligned(reinterpret_cast<uintptr_t>(p), a);
  }
  inline bool is_aligned(volatile void* p, size_t a) {
    return is_aligned(reinterpret_cast<uintptr_t>(p), a);
  }
  inline bool is_aligned(const volatile void* p, size_t a) {
    return is_aligned(reinterpret_cast<uintptr_t>(p), a);
  }

  inline void* align_up(void* p, size_t a) {
    return reinterpret_cast<void*>(align_up(reinterpret_cast<uintptr_t>(p), a));
  }
  inline const void* align_up(const void* p, size_t a) {
    return reinterpret_cast<const void*>(align_up(reinterpret_cast<uintptr_t>(p), a));
  }
  inline volatile void* align_up(volatile void* p, size_t a) {
    return reinterpret_cast<volatile void*>(align_up(reinterpret_cast<uintptr_t>(p), a));
  }
  inline const volatile void* align_up(const volatile void* p, size_t a) {
    return reinterpret_cast<const volatile void*>(align_up(reinterpret_cast<uintptr_t>(p), a));
  }

  inline void* align_down(void* p, size_t a) {
    return reinterpret_cast<void*>(align_down(reinterpret_cast<uintptr_t>(p), a));
  }
  inline const void* align_down(const void* p, size_t a) {
    return reinterpret_cast<const void*>(align_down(reinterpret_cast<uintptr_t>(p), a));
  }
  inline volatile void* align_down(volatile void* p, size_t a) {
    return reinterpret_cast<volatile void*>(align_down(reinterpret_cast<uintptr_t>(p), a));
  }
  inline const volatile void* align_down(const volatile void* p, size_t a) {
    return reinterpret_cast<const volatile void*>(align_down(reinterpret_cast<uintptr_t>(p), a));
  }

  template <typename T>
    inline auto align_up(T p, size_t a)
    -> typename std::enable_if<std::is_pointer<T>::value, T>::type {
      return reinterpret_cast<T*>(align_up(reinterpret_cast<typename __carry_cv<typename std::remove_pointer<T>::type,void>::type*>(p), a));
    }
  template <typename T>
    inline auto align_down(T p, size_t a)
    -> typename std::enable_if<std::is_pointer<T>::value, T>::type {
      return reinterpret_cast<T*>(align_up(reinterpret_cast<typename __carry_cv<typename std::remove_pointer<T>::type,void>::type*>(p), a));
    }

  template <typename T, typename U>
    inline auto align_up_cast(U* p, size_t a=alignof(typename std::remove_pointer<T>::type))
    -> typename std::enable_if<std::is_pointer<T>::value, T>::type {
      return align_up(reinterpret_cast<T>(p), a);
    }

  template <typename T, typename U>
    inline auto align_down_cast(U* p, size_t a=alignof(typename std::remove_pointer<T>::type))
    -> typename std::enable_if<std::is_pointer<T>::value, T>::type {
      return align_down(reinterpret_cast<T>(p), a);
    }

} //namespace std

#endif
