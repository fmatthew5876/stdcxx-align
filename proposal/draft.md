Alignment helpers for C++
==========================================

* Document Number: NXXXX
* Date: 2014-08-20
* Programming Language C++, Library Evolution Working Group
* Reply-to: Matthew Fioravante <fmatthew5876@gmail.com>

The latest draft, reference header, and links to past discussions on Github: 

* <https://github.com/fmatthew5876/stdcxx-align>

Introduction
=============================

This proposal adds support for a few mnemonics which are useful for
low level code which has to manually align blocks of memory. It also adds two
new alignment casts, which allow safe alignment up or down conversions between differently typed pointers.

This proposal was originally a small part of \[[N3864](#N3864)\] but has been broken out as it is somewhat unrelated to the core
purpose of that paper.

This proposal is intended for a C++ Technical Specification.

Impact on the standard
=============================

This proposal is a pure library extension. 
It does not require any changes in the core language and
does not depend on any other library extensions.
The proposal is composed entirely of free functions. The
proposed functions are added to the `<memory>` header.
No new headers are introduced and the implementations 
of these functions on common modern platforms are trivial.

Motivation and Design
================

Manually aligning blocks of memory is an operation often required
in low level applications such as memory allocators, simd code, device
drivers, compression routines, encryption, and binary IO.

The operations `is_aligned()`, `align_up()`, and `align_down()`
are commonly re-implemented over and over again as macros in C and/or
inline template functions in C++.

We propose to standardize these 3 simple mnemonics for the following
reasons:

* They are fundamental low level building blocks which are universally applicable to a wide variety of domains.
* There are many applications which could take advantage of these functions today.
* By providing standard library implementations, users do not have to google, write, test, and maintain their own when they need them.
* These are so trivial to implement on most platforms that the an example implementation is included in this paper.
* Implementation-defined behaviors allow implementors to provide deeper access into platform specific memory model.

Current state of the art
=============================

We currently have `std::align` in the standard for doing alignment calculations.
The function `std::align`
has one specific use case, that is to carve out an aligned buffer of a known size within a larger buffer.
In order to use `std::align`, the user must a priori know the size of the aligned buffer
they require. Unfortunately in some use cases, even calculating the size of this buffer
as an input to `std::align` itself requires doing alignment calculations.
Consider the following example of using aligned SIMD registers to process a memory buffer.
The alignment calculations here cannot be done with `std::align`.

    void process(char* b, char* e) {
      char* pb = std::min((char*)std::align_up(b, sizeof(simd16)), e);
      char* pe = (char*)std::align_down(e, sizeof(simd16));
    
      for(char* p = b; p < pb; ++p) {
        process1(p);
      }
      for(char* p = pb; p < pe; p += sizeof(simd16)) {
        simd16 x = simd16_aligned_load(p);
        process16(x);
        simd16_aligned_store(x, p);
      }
      for(char* p = pe; p < e; ++p) {
        process1(p);
      }
    }

We conclude that `std::align` is much too specific for general alignment calculations. It has a narrow
use case and should only be considered as a helper function for when that use case is needed.
`std::align` could also be implemented using this proposal.

Implementation vs Undefined behavior
--------------------------

Consider the following code fragment

    void process(char* ptr, char* end) {
       char* simd_ptr = std::align_down(ptr);
       char* simd_end = std::align_down(end);

       int128_t simd;
       simd = simd_load_aligned(simd_ptr);
       simd &= mask_leading_bits(ptr-simd_ptr);
       process16(simd_ptr);
       simd_ptr += sizeof(int128_t);

       for(;simd_ptr < simd_end; ++simd_ptr) {
	 simd = simd_load_aligned(simd_ptr);
         process16(simd_ptr);
       }
       simd = simd_load_aligned(simd_ptr);
       simd_ptr &= mask_trailing_bits(end - simd_ptr);
       process16(simd_ptr);
    }

If implemented by hand today in C++, this code would trigger
undefined behavior if `simd_ptr` and/or `simd_end` were located
outside of the *memory block* containing `ptr`.

In general, undefined behavior is a reasonable expectation because
`simd_ptr` could now point to an invalid or restricted memory address.
On most modern machines, one can read and write to any memory address
within the same memory page. Each page is aligned to and is of size `PAGE_SIZE`,
which is commonly 4096 bytes. Since simd registers are typically
nowhere near this size, on these implementations one can safely `align_down` or
`align_up` and arbitrary pointer.

Terminology
=======================

When we say a *memory block*, we mean a block of valid memory allocated to the application either on the stack, heap, or elsewhere.
As is described in &sect;5.7/5 \[[IsoCpp](#IsoCpp)\], the valid set of addresses in this block span from the `first address`
of the block up to and including `1 + the last address`.

Technical Specification
====================

We will now describe the additions to the `<memory>` header. This is a procedural library implemented
entirely using templated and overloaded free functions.
Each pre-condition has
either undefined or implementation defines results and each case is documented below.

## Integral alignment math

For all of the following, `std::is_integral<integral>::value && !std::is_same<integral<bool>::value == true`

    template<class integral>
      constexpr bool is_aligned(integral x, size_t a) noexcept;

Returns `true` if `x == 0` or `x` is a multiple of `a`.

    template<class integral>
      constexpr integral align_up(integral x, size_t a) noexcept;

Returns `n`, where `n` is the least number `>= x` and `is_aligned(n, a)`.

    template<class integral>
      constexpr integral align_down(integral x, size_t a) noexcept;

Returns `n`, where `n` is the greatest number `<= x` and `is_aligned(n, a)`.


### Shared Requirements 

The result is undefined if any of:

* `a == 0`
* `a` is not a power of 2
* `x < 0`
* `integral` is a signed type and the result causes an overflow in either `integral` or its promoted arithmetic type.

The result is 0 if the result is not undefined and any of:

* `integral` is an unsigned type and the result causes an overflow in either `integral` or its promoted arithmetic type.

### Example Implementations

All of these implementations are trivial, efficient, and portable.

    template <class integral>
      constexpr bool is_aligned(integral x, size_t a) noexcept {
        return (x & (integral(a) - 1)) == 0;
      }

    template <class integral>
      constexpr integral align_up(integral x, size_t a) noexcept {
        return integral((x + (integral(a) - 1)) & ~integral(a-1));
      }

    template <class integral>
      constexpr integral align_down(integral x, size_t a) noexcept {
        return integral(x & ~integral(a-1));
      }

* Implementations should support all extended integral types.
* On 2's complement machines `~integral(a-1)` can be optimized to `-integral(a)`.

## Pointer alignment

### Pointer Alignment check

    bool is_aligned(const volatile void* p, size_t a);

Returns `true` if `p == nullptr` or `p` is aligned to `a`, otherwise return `false`.

#### Requirements

The result is undefined if any of:

* `a == 0`
* `a` is not a power of 2
* `a > std::numeric_limits<uintptr_t>::max()` [*note*: this would require a platform where `sizeof(size_t) > sizeof(uintptr_t)` *--end note*].

The result is implementation-defined if the result is not undefined and any of:

* `a > PTRDIFF_MAX`
* `p != nullptr` and `p` does not point to an existing *memory block*
* The result does not reside within the same *memory block* as `p`
    
### Pointer alignment adjustment

For all of the following `std::is_pointer<pointer>::value == true`

    template <class pointer>
      pointer align_up(pointer p, size_t a);

Returns the least pointer `t` such that `t >= p` and `is_aligned(t, a) == true`, or `nullptr` if `p == nullptr`.

    template <class pointer>
      pointer align_down(pointer p, size_t a);

Returns the greatest pointer `t` such that `t <= p` and `is_aligned(t, a) == true`, or `nullptr` if `p == nullptr`.

    nullptr_t align_up(nullptr_t, size_t) { return nullptr; }
    nullptr_t align_down(nullptr_t, size_t) { return nullptr; }

We also add special overloads for `nullptr_t` because `align_up(nullptr, a)` and `align_down(nullptr, a)` will not compile.

#### Shared Requirements

The result is undefined if any of:

* `a == 0`
* `a` is not a power of 2
* `a > std::numeric_limits<uintptr_t>::max()` [*Note*: this result would require a platform where `sizeof(size_t) > sizeof(uintptr_t)` *--end note*].

The result is implementation-defined if the result is not undefined and any of:

* `a > PTRDIFF_MAX`
* `p != nullptr` and `p` does not point to an existing *memory block*
* The result does not reside within the same *memory block* as `p`
    
### Example Implementations for Pointer Operations

    bool is_aligned(const volatile void* p, size_t a) {
      return is_aligned(reinterpret_cast<uintptr_t>(p), a);
    }

    template <class pointer>
      pointer align_up(pointer p, size_t a) {
        return reinterpret_cast<pointer>(align_up(reinterpret_cast<uintptr_t>(p), a));
      }

    template <class pointer>
      pointer align_down(pointer p, size_t a) {
        return reinterpret_cast<pointer>(align_down(reinterpret_cast<uintptr_t>(p), a));
      }

Note that the above assumes the above assume a flat address space and that arithmetic on `uintptr_t` is
equivalent to arithmetic on `char*`. While these conditions prevail for the majority of modern platforms, 
neither of which is required by the standard. It is entirely possible
for an implementation to perform any transformation when casting `void*` to `uintptr_t` as long the 
transformation can be reversed when casting back from `uintptr_t` to `void*`.

    
## Alignment casts

For all of the following, `std::is_pointer<pointer>::value == true`

These functions are designed to become the standard way of doing a `reinterpret_cast` and an alignment adjustment all in one operation which
can optionally be checked by the implementation for correctness.

    template <class pointer, class U>
      pointer align_up_cast(U* p, size_t a=alignof(typename std::remove_pointer<pointer>::type))

Returns the least pointer `t` where `reinterpret_cast<void*>(t) >= reinterpret_cast<void*>p` and `t` is aligned to `a`, or `nullptr` if `p == nullptr`.

    template <class pointer, class U>
      pointer align_down_cast(U* p, size_t a=alignof(typename std::remove_pointer<pointer>::type))

Returns the greatest pointer `t` where `reinterpret_cast<void*>(t) >= reinterpret_cast<void*>p` and `t` is aligned to `a`, or `nullptr` if `p == nullptr`.


    template <class pointer>
      nullptr_t align_up_cast(nullptr_t, size_t a=1) { (void)a; return nullptr; }

    template <class pointer, class U>
      nullptr_t align_down_cast(nullptr_t, size_t a=1) { (void)a; return nullptr; }

Again, we add `nullptr_t` overloads. 

### Shared Requirements

The result is undefined if any of:

* `a == 0`
* `a` is not a power of 2
* `a > std::numeric_limits<uintptr_t>::max()` [*Note*: this would require a platform where `sizeof(size_t) > sizeof(uintptr_t)` *--end note*].

The result is implementation-defined if the result is not undefined and any of:

* `a > PTRDIFF_MAX`
* `p != nullptr` and `p` does not point to an existing *memory block*
* The result does not reside within the same *memory block* as `p`

### Example implementation

    template <class pointer, class U>
      inline pointer align_up_cast(U* p, size_t a=alignof(typename std::remove_pointer<pointer>::type)) {
        return reinterpret_cast<pointer>(align_up(p, a));
      }

    template <class pointer, class U>
      inline pointer align_down_cast(U* p, size_t a=alignof(typename std::remove_pointer<pointer>::type)) {
        return reinterpret_cast<pointer>(align_down(p, a));
      }

### Compiler cast alignment warnings (-Wcast-align)

Some compilers throw a warning if you cast a type with lesser alignment to a type with greater alignment.

For example, on clang 3.4 \[[clang](#clang)\] with `-Wcast-align` enabled, the following code has a warning: `cast from 'char*' to 'int*' increases required alignment from 1 to 4 [-Wcast-align]`

    int foo(void* f) {
      return *(int*)(char*)f;
    }

Implementations should not fire such warnings for `align_up_cast` or `align_down_cast`.

Use Cases
===============

* Future C++ standardization work on aligned allocators and support for over aligned types.
* Operating system kernels, where pointers are represented or interchangeable with `uintptr_t`
* Device drivers and memory mapped IO
* Custom Memory allocators
* Encryption algorithms
* Hashing algorithms
* IO and serialization
* Simd loops
* Simd aligned loads and stores
* Marshaling data for device drivers
* Formatting data for GPU buffers in Graphics API's such as OpenGL

Usage Examples
----------------

Compute page boundaries using integers:

    uintptr_t addr_in_page = /* something */
    auto page_begin = align_down(addr_in_page, PAGE_SIZE);
    auto page_end = align_up(addr_in_page, PAGE_SIZE);

In a disk device driver, optimize reads and write from buffers which are already block size aligned.

    void* user_buffer = /* something */
    if(is_aligned(user_buffer, BLOCK_SIZE) {
       fast_direct_write(user_buffer, size, block_device);
    } else {
       slow_buffered_write(user_buffer, size, block_device);
    }
   
Cast array of ints to array of simd ints.

    int32_t* x = /* something */
    auto* s = align_up_cast<int128_t*>(x);

Get a pointer of type `float*` which is aligned for 16 byte floating operations

    float* f = /* something */
    f = align_up(f, 16);
    

External usage
================

* Linux kernel alignment macros in C: `IS_ALIGNED`, `ALIGN_MASK`, and `ALIGN` \[[LXR](#LXR)\].

Acknowledgments
====================

* Thank you to everyone who has been credited by N3864 for also helping with this proposal.
* Special thanks for Melissa Mears, Thiago Macieira, and David Krauss for their help in discussing issues and helping to improve this proposal.


References
==================

* <a name="N3864"></a>[N3864] Fioravante, Matthew *N3864 - A constexpr bitwise operations library for C++*, Available online at <https://github.com/fmatthew5876/stdcxx-bitops>
* <a name="LXR"></a>[LXR] *Linux/include/linux/kernel.h* Available online at <http://lxr.free-electrons.com/source/include/linux/kernel.h#L50>
* <a name="IsoCpp"></a>[IsoCpp] *ISO C++ standard*
* <a name="clang"></a> *"clang" C Language Family Frontend for LLVM* Available online at <http://clang.llvm.org/>
