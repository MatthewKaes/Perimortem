// Perimortem Engine
// Copyright © Matt Kaes

//
// Perimortem Types
//
/*
  Sized types used for LLVM compatability. This includes the standard definition
  for all Tetrodotoxin types. All types have a defined exact storage size.

  For ease of use this is the only that is not wrapped in a namespace.

  Technically supports LP32 but really only LLP64 and LP64 are used as systems
  under test.
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

using SignedBits_8 = signed char;
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
using Bool = Bits_8;
using Half = SignedBits_16;
using UHalf = Bits_16;
using Int = SignedBits_32;
using UInt = Bits_32;
using Long = SignedBits_64;
using Count = Bits_64;

// Type helpers
template <typename storage>
constexpr Bits_64 size_in_bits() {
  return sizeof(storage) * 8;
}

// Provide the blessed memory operations so LLVM can properly optimize.
extern "C" {
typedef __SIZE_TYPE__ size_t;
extern void* memcpy(void* __restrict dest,
                    const void* __restrict src,
                    size_t count) noexcept(true);

extern int memcmp(const void* a, const void* b, size_t count) noexcept(true);

extern void* memset(void* source, int value, size_t count) noexcept(true);

extern void* malloc(size_t count);
extern void free(void*);
}

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
