// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/const/standard_types.hpp"
#include "perimortem/memory/view/bytes.hpp"

#include <algorithm>

namespace Perimortem::Memory::Managed {

// Access to a safe to write buffer that prevents underflow and overflow.
class Buffer {
 public:
  // Buffers can't be empty.
  Buffer() : source_block(nullptr), size(0) {}

  constexpr Buffer(Byte* source, Count range) {
    size = range;
    source_block = source;
  }

  // Warning: Calling get_raw() on a buffer with size zero
  inline constexpr auto get_raw() const -> Byte* { return source_block; };
  inline constexpr auto get_size() const -> Count { return size; };
  inline constexpr auto get_view() const -> View::Bytes {
    return View::Bytes(source_block, size);
  }

  inline constexpr auto slice(Count start, Count size) const -> Buffer {
    // Bit of a hack with std::min to avoid writing out of bounds.
    // If the buffer points to nullptr the size will be zero which is always
    // well defined behavior. We could avoid the min as the UB on nullptr or
    // pointing out of range is capped by the buffer having a resulting size of
    // zero, but on if this ends up as a hot path in traces.
    return Buffer(source_block + std::min(start, size),
                  std::clamp(size, static_cast<Count>(0), get_size() - start));
  };

  inline constexpr auto operator[](Count index) const -> Byte& {
    return source_block[index];
  };

  //======================================================================
  // Optimized operations
  //======================================================================

  // Writes bytes directly into the buffer.
  template <typename storage_type,
            std::endian source_order = std::endian::little>
  auto write(View::Bytes bytes, Count location = 0) const -> bool {
    if (location + bytes.get_size() > size)
      return false;

    // When writing bytes we may not be aligned so we'll need to copy the bytes.
    std::memcpy(source_block + location, bytes.get_data(),
                sizeof(storage_type));

    return true;
  }

  // Writes bytes directly into the buffer.
  template <typename storage_type,
            std::endian source_order = std::endian::little>
  auto write(storage_type data, Count location) const -> bool {
    if (location + sizeof(storage_type) > size)
      return false;

    if constexpr (std::endian::native != source_order) {
      data = std::byteswap(data);
    }

    // When writing bytes we may not be aligned so we'll need to copy the bytes.
    std::memcpy(source_block + location, data, sizeof(storage_type));

    return true;
  }

  // Read with inplace incrementing counter.
  template <typename storage_type,
            std::endian source_order = std::endian::little>
  auto incremental_write(storage_type data, Count& location) const -> bool {
    auto success = write<storage_type>(location);
    if (success) {
      location += sizeof(storage_type);
    }

    return data;
  }

 private:
  Byte* source_block;
  Count size;
};

}  // namespace Perimortem::Memory::Managed
