// Perimortem Engine
// Copyright © Matt Kaes

//
// Perimortem Standard Library and Compiler Interface
//
/*
  ==============================================================================

                               WHAT IS THIS?

  ==============================================================================

  Perimortem provides a compact runtime and standard-library surface with
  explicit control over data layout, allocation, and dependencies. Avoiding the
  broad C++ standard library also keeps compile times low while keeping the ABI
  and memory management layers fully controlled.

  This file provides the fundamental types used throughout the runtime while
  remaining compatible with C++ standard-library headers when interoperability
  is useful. The actual Perimortem avoids using the STL as headers add both
  substantial build cost and leak complexity into the system.

  ==============================================================================

                                WHY C++?

  ==============================================================================

  While Perimortem does not use much of the C++ standard library it does use the
  C++ language feautres to leverage existing toolchains. By limiting usage to C
  headers we get the benefit of C++ compiler features while sticking to C's
  low-level data and ABI boundaries where it matters: simple enough for callers
  written in other languages to use the runtime while keeping the implementation
  concise and statically checked.

*/

#pragma once

// Unsigned Integers
using Unsigned_8 = unsigned char;
using Unsigned_16 = unsigned short int;
// Legacy support for LP32
#ifdef __LP32__
using Unsigned_32 = unsigned long;
#else
using Unsigned_32 = unsigned int;
#endif
using Unsigned_64 = unsigned long long;

using Signed_8 = signed char;
using Signed_16 = signed short int;
// Legacy support for LP32
#ifdef __LP32__
using Signed_32 = signed long;
#else
using Signed_32 = signed int;
#endif
using Signed_64 = signed long long;

using Real_32 = float;
using Real_64 = double;
using Real_128 = long double;

// Definition for all used based types.
using Count = Unsigned_64;

// Cpp interop
using CppSize = __SIZE_TYPE__;

// In Perimortem boolean values are always 8 bit and treated as unsigned.
//
// The C++ standard leaves it up to the compiler to define size of bool.
// Make a type that will convert between values.
//
// This also specializes boolean operations so they don't alias Unsigned_8.
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

  constexpr auto sign() const -> Signed_64 { return value ? 1 : -1; }
  Unsigned_8 value;
};

// True value that prevents implicit conversion to int.
constexpr Bool True = Bool(true);
// False value that prevents implicit conversion to int.
constexpr Bool False = Bool(false);

// Ensure the data model is correct.
static_assert(sizeof(Unsigned_8) == 1);
static_assert(sizeof(Unsigned_16) == 2);
static_assert(sizeof(Unsigned_32) == 4);
static_assert(sizeof(Unsigned_64) == 8);
static_assert(sizeof(Unsigned_8) == sizeof(Signed_8));
static_assert(__is_same(Signed_8, signed char));
static_assert(sizeof(Unsigned_16) == sizeof(Signed_16));
static_assert(sizeof(Unsigned_32) == sizeof(Signed_32));
static_assert(sizeof(Unsigned_64) == sizeof(Signed_64));
static_assert(sizeof(Real_32) == 4);
static_assert(sizeof(Real_64) == 8);
static_assert(sizeof(Real_128) == 16);

static_assert(sizeof(Bool) == 1);

#ifndef _NEW
constexpr void* operator new(CppSize size, void* ptr) noexcept {
  return ptr;
}
#endif
