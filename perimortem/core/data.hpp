// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

// Provide the blessed memory operations so LLVM can properly optimize.
extern "C" {
typedef CppSize size_t;
extern void* memcpy(void* __restrict dest,
                    const void* __restrict src,
                    size_t count) noexcept(true);

extern int memcmp(const void* a, const void* b, size_t count) noexcept(true);
}  // extern "C"

namespace Perimortem::Core::Data {

enum class ByteOrder {
  Little = __ORDER_LITTLE_ENDIAN__,
  Big = __ORDER_BIG_ENDIAN__,
  Native = __BYTE_ORDER__,
};

// Type helpers
template <typename storage>
consteval auto size_in_bits() -> Bits_64 {
  return sizeof(storage) * 8;
}

template <typename type, Count size>
consteval auto array_size(const type (&)[size]) -> Count {
  return size;
}

// Used for reading const raw data.
template <typename target_type>
auto cast(const Byte* source) -> const target_type* {
  // TODO: start_lifetime_as
  return reinterpret_cast<const target_type*>(source);
}

// Used for reading raw data.
template <typename target_type>
auto cast(Byte* source) -> target_type* {
  // TODO: start_lifetime_as
  return reinterpret_cast<target_type*>(source);
}

template <typename storage_type>
auto copy(Byte* dest, const storage_type* src, Count count = 1)
    -> storage_type* {
  return reinterpret_cast<storage_type*>(
      memcpy(dest, src, Bits_64(sizeof(storage_type)) * count));
}

template <typename storage_type>
constexpr auto compare(const storage_type* dest,
                       const storage_type* src,
                       Count count = 1) -> Bool {
  if consteval {
    for (Count i = 0; i < count; i++) {
      if (dest[i] != src[i]) {
        return false;
      }
    }

    return true;
  } else {
    return memcmp(dest, src, Bits_64(sizeof(storage_type)) * count) == 0;
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

// Swaps two objects
// For consteval move constructors are used.
// For non-consteval a forgetful stack buffer is used to avoid constructing and
// destructing a temporary or calling any move constructors.
template <typename type>
constexpr auto swap(type& a, type& b) -> void {
  if !consteval {
    alignas(alignof(type)) Byte forgetful[sizeof(type)];
    memcpy(&forgetful, &a, sizeof(type));
    memcpy((void*)&a, &b, sizeof(type));
    memcpy((void*)&b, &forgetful, sizeof(type));
  } else {
    auto temp = a;
    a = b;
    b = temp;
  }
}

}  // namespace Perimortem::Core::Data
