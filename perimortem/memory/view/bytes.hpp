// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/const/stack_types.hpp"
#include "perimortem/memory/const/standard_types.hpp"

#include <x86intrin.h>
#include <algorithm>
#include <bit>
#include <cstring>

namespace Perimortem::Memory::View {

// A raw read-only view of bytes with no endianness.
//
// Byte views are typically used as strings but this is only valid when the
// encoding is ASCII. The main purpose of a byte view is to guarantee the data
// lie on 1 byte boundries (for instance ASCII is 7 bit aligned at the byte).
// Any optimized vectorization opperations will break UTF-8 strings unless you
// know exactly what you are doing (scanning for an exact byte).
class Bytes {
 public:
  // Default to empty string.
  constexpr Bytes() {
    size = 0;
    source_block = ""_bytes;
  }

  template <Count element_count>
  constexpr Bytes(Const::StackString<element_count> str) {
    size = str.size();
    source_block = str.data();
  }

  constexpr Bytes(const Bytes&) = default;

  template<std::size_t N>
  constexpr Bytes(const Byte (&source)[N]) {
    size = N;
    source_block = &source;
  }

  constexpr Bytes(const Byte* source, Count source_size) {
    size = source_size;
    source_block = source;
  }

  inline constexpr auto operator==(const Bytes& rhs) const -> bool {
    return rhs.size == size &&
           std::memcmp(source_block, rhs.source_block, size) == 0;
  }

  inline constexpr auto empty() const -> bool { return size == 0; };
  inline constexpr auto get_size() const -> Count { return size; };
  inline constexpr auto get_data() const -> const Byte* {
    return source_block;
  };

  inline constexpr auto slice(Count start, Count size) const -> Bytes {
    return Bytes(source_block + start,
                 std::clamp(size, static_cast<Count>(0), get_size() - start));
  };

  inline constexpr auto operator[](Count index) const -> Byte {
    return source_block[index];
  };

  inline constexpr auto clear() {
    source_block = ""_bytes;
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
        sizeof(storage_type) <= 8,
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

  // Scans a 32 bytes block for the offset of a Byteacter.
  auto scan(Byte search, Count position = 0) const -> Count;

  // Scans a 32 bytes block for the offset of a Byteacter.
  //
  // WARNING: Provides no bounds protection, use only when it's known the block
  // is valid.
  auto fast_scan(Byte search, Count position = 0) const -> Count;

  inline auto block_compare(const Bytes& data, const Count position = 0) const
      -> bool {
    return data.size + position < size &&
           memcmp(data.source_block, source_block + position, data.size) == 0;
  }

 private:
  const Byte* source_block;
  Count size;
};

}  // namespace Perimortem::Memory::View

template<Perimortem::Memory::Const::Bytes view>
constexpr Perimortem::Memory::View::Bytes operator""_view() {
  return Perimortem::Memory::View::Bytes(view.content, view.size);
}
