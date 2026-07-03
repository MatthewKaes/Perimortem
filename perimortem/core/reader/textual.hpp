// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

namespace Perimortem::Core::Reader {

// Reads human-readable values from a text byte buffer. Reads are greedy so
// numeric values must be whitespace seperated to be read appropriately.
//
// Numeric and boolean reads automatically skip leading whitespace except for
// read_byte() which never skips whitespace and always consume exactly one byte
// from the current position.
//
// On overflow or a parse failure the reader enters an invalid state and
// subsequent reads return zero-initialized values without advancing the
// cursor.
class Textual {
 public:
  constexpr Textual(View::Bytes source) : source(source) {}
  constexpr Textual(const Textual& rhs) : source(rhs.source) {}

  // Sets the location of the read cursor.
  // If the index is out of range the cursor is put to the end of the buffer.
  auto set_pointer(Count location) -> void;

  auto read_byte() -> Bits_8;
  auto read_flag() -> Bool;
  auto read_unsigned() -> Bits_64;
  auto read_signed() -> Signed_64;
  auto read_real_32() -> Real_32;
  auto read_real_64() -> Real_64;

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
  auto skip_whitespace() -> void;
  View::Bytes source;
  Count cursor = 0;
  Bool valid_state = True;
};

}  // namespace Perimortem::Core::Reader
