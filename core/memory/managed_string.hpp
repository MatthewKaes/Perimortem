// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "memory/arena.hpp"

#include <x86intrin.h>
#include <bit>
#include <cstring>
#include <string_view>

namespace Perimortem::Memory {

// References a managed string, or creates a managed string in a arena.
//
// The life time of a managed string is assumed to be less than the string that
// it references, even if they don't both exist in the same arena arena.
class ManagedString {
 public:
  // Default to empty string.
  ManagedString() {
    size = 0;
    rented_block = "";
  }

  ManagedString(const ManagedString&) = default;

  // Convert a possibly non-managed string into a managed string.
  ManagedString(Arena& arena, const std::string_view& source) {
    auto buffer = reinterpret_cast<char*>(arena.allocate(source.size()));
    std::memcpy(buffer, source.data(), source.size());

    size = source.size();
    rented_block = buffer;
  }

  // The string is already managed.
  ManagedString(const std::string_view& source) {
    size = source.size();
    rented_block = source.data();
  }

  // The string is already managed.
  ManagedString(const char* source, uint64_t source_size) {
    size = source_size;
    rented_block = source;
  }

  inline constexpr auto operator==(const ManagedString& rhs) const -> bool {
    return rhs.size == size &&
           std::memcmp(rented_block, rhs.rented_block, size) == 0;
  }

  inline constexpr auto operator==(const std::string_view& rhs) const -> bool {
    return rhs.size() == size &&
           std::memcmp(rented_block, rhs.data(), size) == 0;
  }

  inline constexpr auto empty() const -> bool { return size == 0; };

  inline constexpr auto get_size() const -> uint32_t { return size; };

  inline constexpr auto get_view() const -> std::string_view {
    return std::string_view(rented_block, size);
  };

  inline constexpr auto slice(uint64_t start, uint64_t size) const
      -> ManagedString {
    return ManagedString(rented_block + start, size);
  };

  inline constexpr auto operator[](uint64_t index) const -> char {
    return rented_block[index];
  };

  inline constexpr auto clear() {
    rented_block = "";
    size = 0;
  };

  //======================================================================
  // Optimized operations
  //======================================================================

  // Scans a 32 bytes block for the offset of a character.
  auto scan(uint8_t search, const uint32_t position = 0) const -> uint32_t;

  // Scans a 32 bytes block for the offset of a character.
  //
  // WARNING: Provides no bounds protection, use only when it's known the block
  // is valid.
  auto fast_scan(uint8_t search, const uint32_t position = 0) const -> uint32_t;

  inline auto block_compare(const ManagedString& data,
                            const uint32_t position = 0) const -> bool {
    return data.size + position < size &&
           std::memcmp(data.rented_block, rented_block + position, data.size) ==
               0;
  }

 private:
  const char* rented_block;
  uint64_t size;
};

}  // namespace Perimortem::Memory
