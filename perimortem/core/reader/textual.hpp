// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

namespace Perimortem::Core::Reader {

// Reads human-readable values from a text byte buffer. Reads are greedy so
// numeric values must be whitespace seperated to be read appropriately.
//
// Real, Flag, Unsigned, and Signed reads automatically skip leading whitespace.
//
// An overflow or parse failure set the reader to an invalid state and
// subsequent reads return zero-initialized values without advancing the
// cursor.
class Textual {
 public:
  constexpr Textual(View::Bytes source) : source(source) {}
  constexpr Textual(const Textual& rhs) : source(rhs.source) {}

  // Sets the location of the read cursor.
  //
  // An out-of-range location invalidates the reader by setting the position to
  // Count(-1), so using `set_pointer(Count(-1))` is a cheap way to manually
  // invalidate a reader.
  constexpr auto set_location(Count location) -> void { cursor = location; }
  constexpr auto get_location() const -> Count { return cursor; }

  auto read_byte() -> Bits_8;
  auto read_flag() -> Bool;
  auto read_unsigned() -> Bits_64;
  auto read_signed() -> Signed_64;
  auto read_real_32() -> Real_32;
  auto read_real_64() -> Real_64;

  constexpr auto get_size() const -> Count { return source.get_size(); }
  constexpr auto has_content() const -> Bool {
    return cursor < source.get_size();
  }

  constexpr auto reset() -> void { cursor = 0; }

 private:
  View::Bytes source;
  Count cursor = 0;
};

}  // namespace Perimortem::Core::Reader
