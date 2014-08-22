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
low level code which has to manually align blocks of memory. It also adds two
new alignment cast, which allow safe alignment up or down conversions between typed pointers.

Impact on the standard
=============================

This proposal is a pure library extension. 
It does not require any changes in the core language and
does not depend on any other library extensions.
The proposal is composed entirely of free functions. The
proposed functions are added to the `<memory>` header.
No new headers are introduced and the implementations 
of these functions on common modern platforms are trivial.

Motivation
================

Manually aligning blocks of memory is an operation often required
in low level applications such as memory allocators, simd code, device
drivers, compression routines, encryption, and binary IO.

The operations `is_aligned()`, `align_up()`, and `align_down()`
are commonly reimplemented over and over again as macros in C and/or
inline template functions in C++.

We propose to standardize these 3 simple mnemonics for the following
reasons:

* They are fundamental low level building blocks which are universally applicable to a wide variety of domains.
* There are many applications which could take advantage of these functions today.
* By providing standard library implementations, users do not have to google, write, test, and maintain their own when they need them.
* These are so trivial to implement that the full implementation is included in this paper.

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

Terminology and Background
=======================

When we say a *memory block*, we mean a block of valid memory allocated to the application either on the stack, heap, or elsewhere.
As is described in 5.7.5 \[[IsoCpp](#IsoCpp)\], the valid set of addresses in this block span from the first address inside
of the block up to and including 1 + the last address in the memory block.

Technical Specification
====================

We will now describe the additions to the `<memory>` header. This is a procedural library implemented
entirely using templated and overloaded free functions.
For functions which have pre-conditions on their inputs, we
have opted for maximal efficiency with unchecked narrow contracts. Each pre-condition has
either undefined or implementation defines results and each case is documented below.

The template arguments for each proposed function are named `integral` to indicate generic support
for all builtin integral types, signed and unsigned. This also includes implementation specific
wider integral types. All of the functions in this proposal
accept signed integers but have undefined results for negative numbers.
The example implementations depend on a 2's complement representation but implementability of this proposal does not.

## Alignment math (int)

For all of the following, `std::is_integral<integral>::value == true`

    //Returns true if x is a multiple of a
    template<typename integral>
      constexpr bool is_aligned(integral x, size_t a) noexcept;

    //Returns n, where n is the smallest number >= x and is_aligned(n, a)
    template<typename integral>
      constexpr integral align_up(integral x, size_t a) noexcept;

    //Returns n, where n is the largest number <= x and is_aligned(n, a)
    template<typename integral>
      constexpr integral align_down(integral x, size_t a) noexcept;


### Shared Pre-Conditions 

The result is undefined if:

* `a == 0`
* `a` is not a power of 2
* `x < 0`

### Example Implementations

All of these implementations are trivial, efficient, and portable.

    <template <typename integral>
      constexpr bool is_aligned(integral x, size_t a) noexcept {
        return (x & (integral(a) - 1)) == 0
      }

    <template <typename integral>
      constexpr integral align_up(integral x, size_t a) noexcept {
        return (x + (integral(a) - 1)) & -integral(a);
      }

    <template <typename integral>
      constexpr integral align_down(integral x, size_t a) noexcept {
        return x & -integral(a);
      }

NOTE: Implementations should support all implementation specific extended integral types.

## Untyped pointer alignment adjustment (void\*)

    bool is_aligned(void* p, size_t a);
    bool is_aligned(const void* p, size_t a);
    bool is_aligned(volatile void* p, size_t a);
    bool is_aligned(const volatile void* p, size_t a);

    void* align_up(void* p, size_t a);
    void* align_up(const void* p, size_t a);
    void* align_up(volatile void* p, size_t a);
    void* align_up(const volatile void* p, size_t a);

    void* align_down(void* p, size_t a);
    void* align_down(const void* p, size_t a);
    void* align_down(volatile void* p, size_t a);
    void* align_down(const volatile void* p, size_t a);


### Shared Pre-Conditions

The result is undefined if:

* `a == 0`
* `a` is not a power of 2
* `p == nullptr`
    * Alternative: `is_aligned(nullptr) == false`
    * Alternative: `align_up(nullptr) == align_down(nullptr) == nullptr`
        * The alternatives may burden the efficiency of the implementation

The result is implementation defined if:

* `a >= PTRDIFF_MAX`
* `p` does not point to an existing *memory block*
* The result does not reside within the same *memory block* as `p`

### Example implementations

For most modern systems, the pointer variants can be trivially implemented by casting to and from `uintptr_t`

    bool is_aligned(void* p, size_t a) {
      return is_aligned(reinterpret_cast<uintptr_t>(p));
    }

    void* align_up(void* p, size_t a) {
      return reinterpret_cast<void*>(align_up(reinterpret_cast<uintptr_t>(p)));
    }

    void* align_down(void* p, size_t a) {
      return reinterpret_cast<void*>(align_down(reinterpret_cast<uintptr_t>(p)));
    }

Note that the above assumes the above assume a flat address space and that arithmetic on `uintptr_t` is
equivalent to arithmetic on `char*`. Neither of which is required by the standard. It is entirely possible
for an implementation to perform any transformation when casting `void*` to `uintptr_t` as long the 
transformation can be reversed when casting back from `uintptr_t` to `void*`.

## Typed pointer alignment adjustment (T\*)

The following functions adjust the alignment of a typed pointer.

    //Returns the pointer t such that t >=p and is_aligned(t, a) == true
    template <typename T>
      T* align_up(T* p, size_t a)

    //Returns the pointer t such that t <=p and is_aligned(t, a) == true
    template <typename T>
      T* align_down(T* p, size_t a)

The following are also required:

    template <> void* align_up<void*>(void* p, size_t a) { align_up(p, a); } //calls the overloaded version
    template <> const void* align_up<const void*>(const void* p, size_t a) { align_up(p, a); } //calls the overloaded version
    template <> volatile void* align_up<volatile void*>(volatile void* p, size_t a) { align_up(p, a); } //calls the overloaded version
    template <> const volatile void* align_up<const volatile void*>(const volatile void* p, size_t a) { align_up(p, a); } //calls the overloaded version

    template <> void* align_down<void*>(void* p, size_t a) { align_down(p, a); } //calls the overloaded version
    template <> const void* align_down<const void*>(const void* p, size_t a) { align_down(p, a); } //calls the overloaded version
    template <> volatile void* align_down<volatile void*>(volatile void* p, size_t a) { align_down(p, a); } //calls the overloaded version
    template <> const volatile void* align_down<const volatile void*>(const volatile void* p, size_t a) { align_down(p, a); } //calls the overloaded version

### Shared Pre-conditions

The results are undefined if:

* `a < alignof(T)`,
* `a == 0`
* `a` is not a power of 2
* `p == nullptr`

The results are implementation defined if:

* `a >= PTRDIFF_MAX`
* `p` does not point to an existing *memory block*
* The result does not reside within the same *memory block* as `p`
    
### Example Implementations

    template <typename T>
      T* align_up(T* p, size_t a) {
        return reinterpret_cast<T*>(align_up<void*>(p, a)); 
      }

    template <typename T>
      T* align_down(T* p, size_t a) {
        return reinterpret_cast<T*>(align_down<void*>(p, a)); 
      }
    
## Alignment casts (U\*) -> (T\*)

    //Returns the smallest pointer t of type T* where reinterpret_cast<void*>(t) > reinterpret_cast<void*>p and t is aligned to a
    template <typename T, typename U>
      T* align_up_cast<T*>(U* p, size_t a=alignof(T));

    //Returns the smallest pointer t of type T* where reinterpret_cast<void*>(t) > reinterpret_cast<void*>p and t is aligned to a
    template <typename T, typename U>
      T* align_down<T*>(U* p, size_t a=alignof(T));

These functions are designed to become the standard way of doing a `reinterpret_cast` and an alignment adjustment all in one operation which
can optionally be checked by the implementation for correctness.

### Shared Pre-conditions

The results are undefined if:

* `a < alignof(T)`
* `a == 0`
* `a` is not a power of 2
* `p == nullptr`

The results are implementation defined if:

* `a >= PTRDIFF_MAX`
* `p` does not point to an existing *memory block*
* The result does not reside within the same *memory block* as `p`

### Example implementation

    template <typename U, typename T=U>
      T align_up_cast<T>(U* p, size_t a=alignof(T)) {
        return reinterpret_cast<T>(align_up(p, a));
    }

    template <typename U, typename T=U>
      T align_down<T>(U* p, size_t a=alignof(T)) {
        return reinterpret_cast<T>(align_up(p, a));
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

* Operating system kernels, where pointers are represented or interchangable with `uintptr_t`
* Device drivers and memory mapped IO
* Custom Memory allocators
* Encryption algorithms
* Hashing algorithms
* IO and serialization
* Simd lopps
* Simd aligned loads and stores
* Marshalling data for device drivers
* Formatting data for GPU buffers in Graphics API's like OpenGL

Usage Examples
----------------

Compute page boundaries using integers:

    uintptr_t addr_in_page = /* something */
    auto page_begin = align_down(addr_in_page, PAGE_SIZE);
    auto page_end = align_up(addr_in_page, PAGE_SIZE);

In a disk device driver, optimize reads and write from buffers where are already block size aligned.

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

This mini-proposal was originally part of \[[N3864](#N3864)\] but has been broken out as it is somewhat unrelated to the core
purpose of that paper. Thank you to everyone who has been credited by N3864 for also helping with this proposal.

Special thanks to everyone on the std-proposals forum for their valuable insight and feedback.

References
==================

* <a name="N3864"></a>[N3864] Fioravante, Matthew *N3864 - A constexpr bitwise operations library for C++*, Available online at <https://github.com/fmatthew5876/stdcxx-bitops>
* <a name="LXR"></a>[LXR] *Linux/include/linux/kernel.h* Available online at <http://lxr.free-electrons.com/source/include/linux/kernel.h#L50>
* <a name="IsoCpp"></a>[IsoCpp] *ISO C++ standard*
* <a name="clang"></a> *"clang" C Language Family Frontend for LLVM* Available online at <http://clang.llvm.org/>
