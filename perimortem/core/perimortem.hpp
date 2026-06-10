// Perimortem Engine
// Copyright © Matt Kaes

//
// Perimortem Standard Library and Compiler Interface
//
/*
  ==============================================================================

                               WHAT IS THIS?

  ==============================================================================

  As C++ has grown as a language it's standard library has gotten quite slow to
  build and carries over a few annoyances from C compatability. The goal is to
  slowly self host all of TTX (maybe even drop the clang toolchain?), mostly for
  sport.

  In the mean time to speed up compile times by over an order of magnitude and
  to give us finer grain control of the stdlib surface area we stub most of the
  C++ standard lib out but we don't want to lose full compatability with the C++
  standard library (at a minimum it's useful for benchmarking and bootstrapping)

  This file attempts to not make Perimortem _incompatable_ with the C++ standard
  library, but provide a limited stub that may be mixed with standard C++ header
  files as necessary. That said, use of C++'s STL is limited as much as possible
  in throughout the project; some includes bloat compile times by seconds all on
  their own in C++26.

  ==============================================================================

                                WHY NOT C?

  ==============================================================================

  TTX is built to target `libc` so the question is why not use C as the base?

  While we don't want many features from the standard library we do make use of
  a large number of C++ _compiler_ features. All Perimortem features must bottom
  out to a compatible C ABI for TTX, but there is a lot we can express in C++
  without full TTX support. Despite it's flaws it's easier for me personally to
  express in C++ that isn't worth the C or LLVM wranging directly, especially
  when we are building an entire parallel with TTX anyway.

*/

#pragma once

// Raw Data
using Bits_8 = unsigned char;
using Bits_16 = unsigned short int;
// Legacy support for LP32
#ifdef __LP32__
using Bits_32 = unsigned long;
#else
using Bits_32 = unsigned int;
#endif
using Bits_64 = unsigned long long;

using SignedBits_8 = char;
using SignedBits_16 = signed short int;
// Legacy support for LP32
#ifdef __LP32__
using SignedBits_32 = signed long;
#else
using SignedBits_32 = signed int;
#endif
using SignedBits_64 = signed long long;

using Real_32 = float;
using Real_64 = double;
using Real_128 = long double;

// Definition for all used based types.
using Byte = Bits_8;
using Half = SignedBits_16;
using UHalf = Bits_16;
using Int = SignedBits_32;
using UInt = Bits_32;
using Long = SignedBits_64;
using ULong = Bits_64;
using Count = Bits_64;

// Cpp interop
using CppSize = __SIZE_TYPE__;

// In Perimortem boolean values are always 8 bit and treated as unsigned.
//
// The C++ standard leaves it up to the compiler to define size of bool.
// Make a type that will convert between values.
//
// This also specializes boolean operations so they don't alias Bits_8.
struct Bool {
  constexpr Bool() : value(false) {}
  constexpr Bool(bool value) : value(value) {}
  constexpr explicit operator bool() const { return value; }
  constexpr auto operator==(Bool rhs) const -> Bool {
    return value == rhs.value;
  }
  constexpr auto operator!=(Bool rhs) const -> Bool {
    return value != rhs.value;
  }
  constexpr auto operator|(Bool rhs) const -> Bool { return value | rhs.value; }
  constexpr auto operator&(Bool rhs) const -> Bool { return value & rhs.value; }
  constexpr auto operator^(Bool rhs) const -> Bool { return value ^ rhs.value; }
  constexpr auto operator!() const -> Bool { return !value; }
  constexpr auto operator&=(Bool rhs) -> Bool& {
    value &= rhs.value;
    return *this;
  }
  constexpr auto operator|=(Bool rhs) -> Bool& {
    value |= rhs.value;
    return *this;
  }
  constexpr auto sign() const -> SignedBits_64 { return value ? 1 : -1; }
  Bits_8 value;
};

// True value that prevents implicit conversion to int.
constexpr Bool True = Bool(true);
// False value that prevents implicit conversion to int.
constexpr Bool False = Bool(false);

// Ensure the data model is correct.
static_assert(sizeof(Bits_8) == 1);
static_assert(sizeof(Bits_16) == 2);
static_assert(sizeof(Bits_32) == 4);
static_assert(sizeof(Bits_64) == 8);
static_assert(sizeof(Bits_8) == sizeof(SignedBits_8));
static_assert(sizeof(Bits_16) == sizeof(SignedBits_16));
static_assert(sizeof(Bits_32) == sizeof(SignedBits_32));
static_assert(sizeof(Bits_64) == sizeof(SignedBits_64));
static_assert(sizeof(Real_32) == 4);
static_assert(sizeof(Real_64) == 8);
static_assert(sizeof(Real_128) == 16);

static_assert(sizeof(Bool) == 1);

#ifndef _NEW
constexpr void* operator new(CppSize size, void* ptr) noexcept {
  return ptr;
}
#endif
