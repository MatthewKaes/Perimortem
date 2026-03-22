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

#include <bit>
#include <cstdint>
#include <stdfloat>

// Raw Data
using Byte = std::uint8_t;
using Bits_8 = Byte;
using Bits_16 = std::uint16_t;
using Bits_32 = std::uint32_t;
using Bits_64 = std::uint64_t;
using Bits_128 = __uint128_t;

// Numerics
using Int = std::int32_t;
using Long = std::int64_t;
using UInt = Bits_32;
using Count = Bits_64;

// Floating point types, use def hack to get around fixed floats not yet in
// C++23 for clang.
#ifndef __STDCPP_FLOAT32_T__
using Real = float;
#else
using Real = std::float32_t;
#endif

#ifndef __STDCPP_FLOAT64_T__
using Real_64 = double;
#else
using Decimal = std::float64_t;
#endif

#ifndef __STDCPP_FLOAT128_T__
using Decimal = long double;
#else
using Decimal = std::float128_t;
#endif

// Type helpers
template <typename storage>
constexpr Bits_64 size_in_bits() {
  return sizeof(storage) * 8;
}

namespace Perimortem::Memory::Const {
// Used to convert string literals into non-null terminated bytes.
template <Count literal_size>
struct Bytes {
  // Remove null terminator.
  Byte content[literal_size - 1]{};
  const Count size = literal_size - 1;

  constexpr Bytes(const char (&source)[literal_size]) {
    for (Count i = 0; i < literal_size - 1; i++) {
      content[i] = source[i];
    }
  }
};
}  // namespace Perimortem::Memory::Const

template <Perimortem::Memory::Const::Bytes converter>
constexpr auto operator""_bytes() {
  return converter.content;
}

constexpr Byte operator""_byte(char ch) {
  return std::bit_cast<Byte>(ch);
}