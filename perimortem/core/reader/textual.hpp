// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

namespace Perimortem::Core::Reader {

// Reads human-readable values from a text byte buffer, mirroring the output
// of Writer::Textual.
//
// Numeric and boolean reads automatically skip leading whitespace (space, tab,
// newline, carriage return). Raw byte reads via read_byte() never skip
// whitespace and always consume exactly one byte from the current position.
//
// On overflow or a parse failure the reader enters an invalid state and
// subsequent reads return zero-initialised values without advancing the
// pointer.
class Textual {
 public:
  constexpr Textual(Core::View::Bytes source) : data(source) {}
  constexpr Textual(const Textual& rhs) : data(rhs.data) {}

  // Sets the location of the read pointer.
  // If the index is out of range the pointer is put to the end of the buffer.
  auto set_pointer(Count location) -> void;

  auto read_byte() -> Byte;
  auto read_flag() -> Bool;
  auto read_half() -> Half;
  auto read_unsigned_half() -> UHalf;
  auto read_int() -> Int;
  auto read_unsigned_int() -> UInt;
  auto read_long() -> Long;
  auto read_unsigned_long() -> ULong;
  auto read_real_32() -> Real_32;
  auto read_real_64() -> Real_64;

  constexpr auto get_size() const -> Count { return data.get_size(); }
  constexpr auto get_location() const -> Count { return ptr_location; }
  constexpr auto is_valid() const -> Bool { return valid_state; }

 private:
  auto skip_whitespace() -> void;
  auto read_real() -> Real_64;
  Core::View::Bytes data;
  Count ptr_location = 0;
  Bool valid_state = True;
};

}  // namespace Perimortem::Core::Reader
