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

// Since Perimortem is exception free the header provides a macro to propagate
// failures up the stack with a slightly less verbose syntax. It's mostly used
// with `Option` but it can be used anywhere a default constructed object is the
// representative failure state.
#define BAIL_IF(...)   \
  do {                 \
    if (__VA_ARGS__) { \
      return {};       \
    }                  \
  } while (false)

// Unsigned Integers
using U8 = unsigned char;
using U16 = unsigned short int;
// Legacy support for LP32
#ifdef __LP32__
using U32 = unsigned long;
#else
using U32 = unsigned int;
#endif
using U64 = unsigned long long;

using S8 = signed char;
using S16 = signed short int;
// Legacy support for LP32
#ifdef __LP32__
using S32 = signed long;
#else
using S32 = signed int;
#endif
using S64 = signed long long;

using R32 = float;
using R64 = double;

// Definition for all used based types.
using Count = U64;

// Cpp interop
using CppSize = __SIZE_TYPE__;

// In Perimortem boolean values are always 8 bit and treated as unsigned.
//
// The C++ standard leaves it up to the compiler to define size of bool.
// Make a type that will convert between values.
//
// This also specializes boolean operations so they don't alias U8.
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

  constexpr auto sign() const -> S64 { return value ? 1 : -1; }
  U8 value;
};

// True value that prevents implicit conversion to int.
constexpr Bool True = Bool(true);
// False value that prevents implicit conversion to int.
constexpr Bool False = Bool(false);

// Ensure the data model is correct.
static_assert(sizeof(U8) == 1);
static_assert(sizeof(U16) == 2);
static_assert(sizeof(U32) == 4);
static_assert(sizeof(U64) == 8);
static_assert(sizeof(U8) == sizeof(S8));
static_assert(__is_same(S8, signed char));
static_assert(sizeof(U16) == sizeof(S16));
static_assert(sizeof(U32) == sizeof(S32));
static_assert(sizeof(U64) == sizeof(S64));
static_assert(sizeof(R32) == 4);
static_assert(sizeof(R64) == 8);

static_assert(sizeof(Bool) == 1);

#ifndef _NEW
constexpr void* operator new(CppSize size, void* ptr) noexcept {
  return ptr;
}
#endif
