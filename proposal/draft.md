Alignment helpers for C++
==========================================

* Document Number: NXXXX
* Date: 2014-08-20
* Programming Language C++, Numerics Working Group
* Reply-to: Matthew Fioravante <fmatthew5876@gmail.com>

The latest draft, reference header, and links to past discussions on github: 

* https://github.com/fmatthew5876/stdcxx-align

Introduction
=============================

This proposal adds support for a few mnemonics which are useful for
low level code which has to manually align blocks of memory.

Impact on the standard
=============================

This proposal is a pure library extension. 
It does not require any changes in the core language and
does not depend on any other library extensions.
The proposal is composed entirely of free functions. The
proposed functions are added to the `<memory>` header.
No new headers are introduced and the implementations 
of these functions on common platforms are trivial.

Motivation
================

Manually aligning blocks of memory is an operation often required
in low level applications such as memory allocators, simd code, device
drivers, compression routines, encryption, and binary IO.

The operations `is_aligned()`, `align_up()`, and `align_down()`
are commonly reimplemented over and over again as macros in C and/or
inline template functions in C++.
For example in the linux kernel,
they have implemented the following corresponding macros in C: `IS_ALIGNED`, `__ALIGN_MASK`, and `ALIGN` \[[LXR](#LXR)\].

We propose to standardize these 3 simple mnemonics for the following
reasons:

* They are fundamental low level building blocks which are universally applicable to a wide variety of domains
* By providing standard library implementations, users do not have to google, write, test, and maintain their own when they need them
* These are so trivial to implement that the full implementation is included in this paper.
* Adding more simple low level tools can help make C++ more popular with the embedded and device driver developer community.

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

Technical Specification
====================

We will now describe the additions to the `<memory>` header. This is a procedural library implemented
entirely using `constexpr` templated free functions.
All functions have been qualified with `noexcept`. These
operations are most often used in highly optimized numerical code where the overhead of exception
handling would be inappropriate. 
For functions which have pre-conditions on their inputs, we
have opted for maximal efficiency with unchecked narrow contracts.

The template arguments for each proposed function are named `integral` to indicate generic support
for all builtin integral types, signed and unsigned. This also includes implementation specific
wider integral types. All of the functions in this proposal
accept signed integers but have undefined results for negative numbers.
The example implementations depend on a 2's complement representation but implementability of this proposal does not.

#### List of Functions

For all of the following functions, the result is undefined if `x < 0` or `align` is not a power of 2.
    
    template <class integral>
    constexpr bool is_aligned(integral x, size_t align) noexcept;

* *Returns:* `true` if `x` is a multiple of `align`.
    
<!-- -->

    bool is_aligned(void* p, size_t align) noexcept;

* *Returns:* `true` if `p` is aligned by `align`.
    
<!-- -->

    template <class integral>
    constexpr integral align_up(integral x, size_t align) noexcept;

* *Returns:* The unique value `n` such that `is_aligned(n, align) && n >= x`.
    
<!-- -->

    void* align_up(void* p, size_t align) noexcept;

* *Returns:* The unique value `np` such that `is_aligned(np, align) && np >= x`.
    
<!-- -->
    
    template <class integral>
    constexpr integral align_down(integral x, size_t align) noexcept;

* *Returns:* The unique value `n` such that `is_aligned(n, align) && n <= x`.
    
<!-- -->

    void* align_down(void* val, size_t align) noexcept;

* *Returns:* The unique value `np` such that `is_aligned(np, align) && np <= x`.

The result of `align_up` or `align_down` with parameters of object pointer type is undefined 
if the resulting pointer value is before or after one past the end of the array or memory block
containing the original pointer. The result of `align_up`, `align_down`, and `is_aligned` is
undefined if `align > PTRDIFF_MAX`.

Example Implementations
============

For the integral types, these alignment functions have trivial implementations:

* `integral align_up(integral x, size_t a)` -> `(x + (integral(a) - 1)) & -integral(a)`
* `integral align_down(integral x, size_t a)` -> `x & -integral(a)`
* `bool is_aligned(integral x, size_t a)` -> `(x & (integral(a) - 1)) == 0`

For most modern systems, the pointer variants can be trivially implemented by casting to `uintptr_t`

* `void* align_up(void* p, size_t a)` -> `reinterpret_cast<void*>(align_up(reinterpret_cast<uintptr_t>(p), a))`
* `void* align_down(void* p, size_t a)` -> `reinterpret_cast<void*>(align_down(reinterpret_cast<uintptr_t>(p), a))`
* `bool is_aligned(void* p, size_t a)` -> `is_aligned(reinterpret_cast<uintptr_t>(p), a)`

Note that the above assumes the above assume a flat address space and that arithmetic on `uintptr_t` is
equivalent to arithmetic on `char*`. Neither of which is required by the standard. It is entirely possible
for an implementation to perform any transformation when casting `void*` to `uintptr_t` as long the 
transformation can be reversed when casting back from `uintptr_t` to `void*`.

Acknowledgments
====================

This mini-proposal was originally part of \[[N3864](#N3864)\] but has been broken out as it is somewhat unrelated to the core
purpose of that paper. Thank you to everyone who has been credited by N3864 for also helping with this proposal.

Special thanks to everyone on the std-proposals forum for their valuable insight and feedback.

References
==================

* <a name="N3864"></a>[N3864] Fioravante, Matthew *N3864 - A constexpr bitwise operations library for C++*, Available online at <https://github.com/fmatthew5876/stdcxx-bitops>
* <a name="LXR"></a>[LXR] *Linux/include/linux/kernel.h* Available online at <http://lxr.free-electrons.com/source/include/linux/kernel.h#L50>
