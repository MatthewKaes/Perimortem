// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/writer/serial.hpp"

namespace Perimortem::Core::Reader {

// Reads self-describing values from a serial byte stream produced by
// Writer::Serial.
//
// Each value in the stream begins with a type tag byte that identifies the
// payload width; a type mismatch between what is in the stream and what was
// requested puts the reader into an invalid state. Blobs carry an additional
// element-type tag and a run-length-encoded size before the payload bytes.
//
// On any error the reader enters an invalid state and all subsequent reads
// return zero-initialised values without advancing the pointer.
class Serial {
 public:
  using DataType = Writer::Serial::DataType;

  constexpr Serial(Core::View::Bytes source) : data(source) {}
  constexpr Serial(const Serial& rhs) : data(rhs.data) {}

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
  auto read_blob() -> Core::View::Bytes;

  constexpr auto get_size() const -> Count { return data.get_size(); }
  constexpr auto get_location() const -> Count { return ptr_location; }
  constexpr auto is_valid() const -> Bool { return valid_state; }

 private:
  Core::View::Bytes data;
  Count ptr_location = 0;
  Bool valid_state = True;
};

}  // namespace Perimortem::Core::Reader
