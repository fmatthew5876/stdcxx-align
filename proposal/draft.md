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
low level code which has to manually align blocks of memory. It also adds 2
new alignment cast, which allow safe alignment conversions between typed pointers.

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

## Alignment math (int)

The integral signatures:

    //Returns true if x is a multiple of a
    template<typename integral>
      constexpr bool is_aligned(integral x, size_t a) noexcept;

    //Returns n, where n is the smallest number >= x and is_aligned(n, a)
    template<typename integral>
      constexpr integral align_up(integral x, size_t a) noexcept;

    //Returns n, where n is the largest number <= x and is_aligned(n, a)
    template<typename integral>
      constexpr integral align_down(integral x, size_t a) noexcept;

### Specification Details

* All signed and unsigned integral types shall be supported.
* Implementations should support all builtin extended integral types.


### Shared Pre-Conditions 

* `a` must be a power of 2, otherwise the result is undefined.
* if `x < 0`, the result is undefined.

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

## Untyped pointer alignment adjustment (void\*)

The `void*` overloads:

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

* If `p == nullptr`, the result is undefined
    * Alternative: `is_aligned(nullptr) == false`
    * Alternative: `align_up(nullptr) == align_down(nullptr) == nullptr`
        * The alternatives may burden the efficiency of the implementation
* Note: a *memory block* as stated below defines a block of valid memory
which extends from the first memory location in the block up until 1 after
the last memory location as described in 5.7.5 of the standard \[[IsoCpp](#IsoCpp)\].
* If `p` does not point to a valid memory block, the result is implementation defined.
* If `align_up(p, a)` or `align_down(p, a)` produces a resulting pointer which does
    not point to p's memory block, the result is implementation defined.
* If `a > PTRDIFF_MAX`, the result is implementation defined.

### Example implementations

For most modern systems, the pointer variants can be trivially implemented by casting to `uintptr_t`

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
    template <> void* align_down<void*>(void* p, size_t a) { align_down(p, a); } //calls the overloaded version
    //Repeat for const void*, volatile void*, and const volatile void*

### Shared Pre-conditions

* `a` must be a power of 2, or results undefined.
* `a >= alignof(T)`, or results undefined
* if `a >= PTRDIFF_MAX`, the results are implementation defined.
* if `p == nullptr`, the results are implementation defined.
* if the result does not reside in the same *memory block* as `p`, the results are implementation defined.
    
### Example Implementations

    template <typename T>
      T* align_up(T* p, size_t a) {
        return reinterpret_cast<T*>(align_up<void*>(p, a)); 
      }

    //Returns the pointer t such that t <=p and is_aligned(t, a) == true
    template <typename T>
      T* align_down(T* p, size_t a) {
        return reinterpret_cast<T*>(align_down<void*>(p, a)); 
      }
    
## Alignment casts (U\*) -> (T\*)

We also provide a set of alignment casts:

For all of these, `std::is_pointer<T>::value == true`

    //Returns the smallest pointer t of type T* where reinterpret_cast<void*>(t) > reinterpret_cast<void*>p and t is aligned to a
    template <typename T, typename U>
      T align_up_cast<T>(U* p, size_t a=alignof(T));

    //Returns the smallest pointer t of type T* where reinterpret_cast<void*>(t) > reinterpret_cast<void*>p and t is aligned to a
    template <typename T, typename U>
      T align_down<T>(U* p, size_t a=alignof(T));

### Shared Pre-conditions

* `a` must be a power of 2, or the result is undefined
* `a >= alignof(T) && a >= alignof(U)`, or the result is undefined
* if `a >= PTRDIFF_MAX`, the results are implementation defined.
* If `p` is not aligned to std::alignof(p), the result is undefined.
* If `p == nullptr`, the result is undefined
* The result must reside within the same *memory block* as `p`, otherwise the result is implementation defined.

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

For example, on clang 3.4 with `-Wcast-align` enabled, the following code has a warning: `cast from 'char*' to 'int*' increases required alignment from 1 to 4 [-Wcast-align]`

    int main() {
      void* f;
      return *(int*)(char*)f;
    }

Implementations should not fire this warning for `align_up_cast` or `align_down_cast`.
These functions should become the standard way to perform alignment on pointers.


Use Cases
===============

* Operating system kernels, where pointers are represented or interchangable with `uintptr_t`
* Device drivers and memory mapped IO
* Custom Memory allocators
* Encryption
* Hashing
* IO and serialization
* Simd lopps
* Simd aligned loads and stores
* Marshalling data for device drivers
* Formatting data for GPU buffers in Graphics API's like OpenGL

Usage Examples
----------------

    //Cast array of ints to array of simd ints.
    int32_t* x = /* something */
    auto* s = align_up_cast<int128_t*>(x);

<!--- -->

    //Get a pointer of type int32_t* that is aligned up by 16 bytes.
    int32_t* x = /* something */
    x = align_up(x, 16);
    


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
