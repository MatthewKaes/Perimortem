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
using Bits_64 = unsigned long long;
#else
using Bits_32 = unsigned int;
using Bits_64 = unsigned long;
#endif

using SignedBits_8 = signed char;
using SignedBits_16 = signed short int;
// Legacy support for LP32
#ifdef __LP32__
using SignedBits_32 = signed long;
using SignedBits_64 = signed long long;
#else
using SignedBits_32 = signed int;
using SignedBits_64 = signed long;
#endif

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
using ULong = Bits_64;
#ifdef __LP32__
using Count = Bits_32;
#else
using Count = Bits_64;
#endif

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
