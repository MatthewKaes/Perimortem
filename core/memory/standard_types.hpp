// Perimortem Engine
// Copyright © Matt Kaes

//
// Perimortem Types
//
/*
  Sized types used for LLVM compatability. This includes the standard definition
  for all Tetrodotoxin types.

  For ease of use this is the only that is not wrapped in a namespace.
*/

#pragma once

#include <cstdint>
#include <stdfloat>

// Raw Data
using Byte = std::uint8_t;
using Bits_8 = Byte;
using Bits_16 = std::uint16_t;
using Bits_32 = std::uint32_t;
using Bits_64 = std::uint64_t;
using Bits_128 = __uint128_t;

template <typename storage>
constexpr Bits_64 size_in_bits() {
  return sizeof(storage) * 8;
}

// Numerics
using Int = std::int32_t;
using Long = std::int64_t;
using UInt = Bits_32;
using Count = Bits_64;

// Floating point types, use def hack to get around fixed floats not yet in
// C++23 for clang.
#ifndef __STDCPP_FLOAT32_T__
using Num = float;
#else
using Num = std::float32_t;
#endif

#ifndef __STDCPP_FLOAT128_T__
using Dec = long double;
#else
using Dec = std::float128_t;
#endif
