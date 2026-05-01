// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/utility/func/math.hpp"

#pragma once

// Provide the blessed memory operations so LLVM can properly optimize.
extern "C" {
typedef __SIZE_TYPE__ size_t;
extern void* memcpy(void* __restrict dest,
                    const void* __restrict src,
                    size_t count) noexcept(true);

extern int memcmp(const void* a, const void* b, size_t count) noexcept(true);
}  // extern "C"

namespace Perimortem::Utility::Func::Data {

enum class ByteOrder {
  Little = __ORDER_LITTLE_ENDIAN__,
  Big = __ORDER_BIG_ENDIAN__,
  Native = __BYTE_ORDER__,
};

// Type helpers
template <typename storage>
constexpr auto size_in_bits() -> Bits_64 {
  return Bits_64(sizeof(storage)) * 8_U64;
}

template <typename target_type>
auto cast(const Byte* source) -> const target_type {
  return __builtin_launder(reinterpret_cast<target_type>(source));
}

template <typename target_type>
auto cast(Byte* source) -> target_type {
  return __builtin_launder(reinterpret_cast<target_type>(source));
}

template <typename storage_type>
auto copy(Byte* dest, const storage_type* src, Count count = 1_U64)
    -> storage_type* {
  return reinterpret_cast<storage_type*>(
      memcpy(dest, src, Bits_64(sizeof(storage_type)) * count));
}

template <typename storage_type>
constexpr auto compare(const storage_type* dest,
                       const storage_type* src,
                       Count count = 1_U64) -> Bool {
  if consteval {
    for (Count i = 0_U64; i < count; i++) {
      if (dest[i.value] != src[i.value]) {
        return False;
      }
    }

    return True;
  } else {
    return Bool(memcmp(dest, src, sizeof(storage_type) * count.value) == 0);
  }
}

template <ByteOrder source, ByteOrder target, typename storage_type>
constexpr auto ensure_endian(storage_type bin) -> storage_type {
  if constexpr (source != target) {
    switch (sizeof(storage_type)) {
      case 1:
        return bin;
      case 2:
        return __builtin_bswap16(bin);
      case 4:
        return __builtin_bswap32(bin);
      case 8:
        return __builtin_bswap64(bin);
    }
  }

  return bin;
}

}  // namespace Perimortem::Utility::Func::Data
