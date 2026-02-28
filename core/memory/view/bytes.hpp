// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "concepts/standard_types.hpp"
#include "memory/arena.hpp"

#include <x86intrin.h>
#include <cstring>

namespace Perimortem::Memory::View {

// A raw read-only view of bytes with no endianness.
//
// Byte views are typically used as strings but this is only valid when the
// encoding is ASCII. The main purpose of a byte view is to gurentee the data
// lie on 1 byte boundries (for instance ASCII is 7 bit aligned at the byte).
// Any optimized vectorization opperations will break UTF-8 strings unless you
// know exactly what you are doing (scanning for an exact byte).
class Bytes {
 public:
  // Default to empty string.
  constexpr Bytes() {
    size = 0;
    source_block = "";
  }

  constexpr Bytes(const Bytes&) = default;

  constexpr Bytes(const char* source) {
    size = std::strlen(source);
    source_block = source;
  }

  constexpr Bytes(const char* source, Count source_size) {
    size = source_size;
    source_block = source;
  }

  constexpr Bytes(const Byte* source, Count source_size) {
    size = source_size;
    source_block = reinterpret_cast<const char*>(source);
  }

  inline constexpr auto operator==(const Bytes& rhs) const -> bool {
    return rhs.size == size &&
           std::memcmp(source_block, rhs.source_block, size) == 0;
  }

  inline constexpr auto operator==(const char* rhs) const -> bool {
    return std::strlen(rhs) == size &&
           std::memcmp(source_block, rhs, size) == 0;
  }

  inline constexpr auto empty() const -> bool { return size == 0; };

  inline constexpr auto get_size() const -> Count { return size; };

  inline constexpr auto get_data() const -> const char* {
    return source_block;
  };

  inline constexpr auto slice(Count start, Count size) const -> Bytes {
    return Bytes(source_block + start, size);
  };

  inline constexpr auto operator[](Count index) const -> char {
    return source_block[index];
  };

  inline constexpr auto clear() {
    source_block = "";
    size = 0;
  };

  //======================================================================
  // Optimized operations
  //======================================================================

  // Writes bytes directly into the buffer.
  template <typename storage_type,
            std::endian source_order = std::endian::little>
  auto read(Count location) const -> storage_type {
    static_assert(
        sizeof(storage_type) < 8,
        "Reading blocks larger than 8 bytes is typically an anti-pattern, "
        "use buffers instead or smaller blocks if you care about endianness.");

    if (location + sizeof(storage_type) > size)
      return storage_type();

    // When reading bytes we may not be aligned so we'll need to copy the bytes.
    storage_type data;
    std::memcpy(&data, source_block + location, sizeof(storage_type));

    if constexpr (std::endian::native != source_order) {
      data = std::byteswap(data);
    }

    return data;
  }

  // Read with inplace incrementing counter.
  template <typename storage_type,
            std::endian source_order = std::endian::little>
  auto incremental_read(Count& location) const -> storage_type {
    storage_type data = read<storage_type>(location);
    location += sizeof(storage_type);
    return data;
  }

  // Scans a 32 bytes block for the offset of a character.
  auto scan(Byte search, Count position = 0) const -> Count;

  // Scans a 32 bytes block for the offset of a character.
  //
  // WARNING: Provides no bounds protection, use only when it's known the block
  // is valid.
  auto fast_scan(Byte search, Count position = 0) const -> Count;

  inline auto block_compare(const Bytes& data,
                            const Count position = 0) const -> bool {
    return data.size + position < size &&
           memcmp(data.source_block, source_block + position, data.size) == 0;
  }

 private:
  const char* source_block;
  uint64_t size;
};

}  // namespace Perimortem::Memory::View

constexpr Perimortem::Memory::View::Bytes operator""_bv(const char* arr,
                                                        size_t size) {
  return Perimortem::Memory::View::Bytes(arr, size);
}
