// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/data.hpp"

namespace Perimortem::Core::Reader {

// Reads typed values from a flat byte buffer with alignment padding between
// fields.
//
// Each read advances the pointer to the next natural alignment boundary for
// that type before consuming bytes. Raw-byte reads via read_bytes() bypass
// alignment and always read from the current position allowing reads from any
// arbitrary location, however it does not convert the bytes into Native endian
// and simply returns a View.
//
// On overflow or any failed read the reader enters an invalid state and all
// subsequent reads return zero-initialized values without advancing the
// pointer.
template <Data::ByteOrder stream_endian>
class Binary {
 public:
  constexpr Binary(View::Bytes source) : data(source) {}
  constexpr Binary(const Binary& rhs) : data(rhs.data) {}

  // Sets the location of the read pointer.
  // If the index is out of range the pointer is put to the end of the buffer.
  auto set_pointer(Count location) -> void;

  auto read_bits_8() -> Bits_8;
  auto read_bits_16() -> Bits_16;
  auto read_bits_32() -> Bits_32;
  auto read_bits_64() -> Bits_64;
  auto read_signed_bits_8() -> SignedBits_8;
  auto read_signed_bits_16() -> SignedBits_16;
  auto read_signed_bits_32() -> SignedBits_32;
  auto read_signed_bits_64() -> SignedBits_64;
  auto read_real_32() -> Real_32;
  auto read_real_64() -> Real_64;
  auto read_bytes(Count count) -> View::Bytes;

  constexpr auto get_size() const -> Count { return data.get_size(); }
  constexpr auto get_location() const -> Count { return ptr_location; }
  constexpr auto is_valid() const -> Bool { return valid_state; }
  constexpr auto is_empty() const -> Bool {
    return get_location() == get_size();
  }
  constexpr auto reset() -> void {
    valid_state = true;
    ptr_location = 0;
  }

 private:
  View::Bytes data;
  Count ptr_location = 0;
  Bool valid_state = True;
};

}  // namespace Perimortem::Core::Reader
