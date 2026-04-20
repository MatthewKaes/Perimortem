// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/math.hpp"

#pragma once

// Provide the blessed memory operations so LLVM can properly optimize.
extern "C" {
typedef __SIZE_TYPE__ size_t;
extern void* memcpy(void* __restrict dest,
                    const void* __restrict src,
                    size_t count) noexcept(true);

extern int memcmp(const void* a, const void* b, size_t count) noexcept(true);

extern void* memset(void* source, int value, size_t count) noexcept(true);

extern void* malloc(size_t count) noexcept(true);
extern void free(void*) noexcept(true);

void* operator new(size_t size, void* ptr) noexcept;
}

namespace Perimortem::Core {

// Type helpers
template <typename storage>
constexpr auto size_in_bits() -> Bits_64 {
  return sizeof(storage) * 8;
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
auto copy(Byte* dest, const storage_type* src, Count count = 1)
    -> storage_type* {
  return reinterpret_cast<storage_type*>(
      memcpy(dest, src, sizeof(storage_type) * count));
}

template <typename storage_type>
constexpr auto compare(const Byte* dest,
                       const storage_type* src,
                       Count count = 1) -> Bool {
  return memcmp(dest, src, sizeof(storage_type) * count) == 0;
}

}  // namespace Perimortem::Core
