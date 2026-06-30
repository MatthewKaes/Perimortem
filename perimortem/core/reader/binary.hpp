// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/data.hpp"

namespace Perimortem::Core::Reader {

// Reads typed values from a dense byte buffer.
//
// Each read consumes exactly the bytes for the requested type from the current
// cursor. The reader never inserts alignment padding, so it can decode packed
// protocol data and subviews that start at arbitrary byte offsets.
//
// On overflow or any failed read the reader enters an invalid state and all
// subsequent reads return zero-initialized values without advancing the cursor.
template <Data::ByteOrder stream_endian>
class Binary {
 public:
  constexpr Binary(View::Bytes source) : source(source) {}
  constexpr Binary(const Binary& rhs) : source(rhs.source) {}

  // Sets the location of the read cursor.
  // If the index is out of range the cursor is put to the end of the buffer.
  auto set_pointer(Count location) -> void;

  auto read_bits_8() -> Bits_8;
  auto read_bits_16() -> Bits_16;
  auto read_bits_32() -> Bits_32;
  auto read_bits_64() -> Bits_64;
  auto read_signed_bits_8() -> Signed_8;
  auto read_signed_bits_16() -> Signed_16;
  auto read_signed_bits_32() -> Signed_32;
  auto read_signed_bits_64() -> Signed_64;
  auto read_real_32() -> Real_32;
  auto read_real_64() -> Real_64;
  auto read_bytes(Count count) -> View::Bytes;

  constexpr auto get_size() const -> Count { return source.get_size(); }
  constexpr auto get_location() const -> Count { return cursor; }
  constexpr auto is_valid() const -> Bool { return valid_state; }
  constexpr auto is_empty() const -> Bool {
    return get_location() == get_size();
  }
  constexpr auto reset() -> void {
    valid_state = true;
    cursor = 0;
  }

 private:
  View::Bytes source;
  Count cursor = 0;
  Bool valid_state = True;
};

}  // namespace Perimortem::Core::Reader
