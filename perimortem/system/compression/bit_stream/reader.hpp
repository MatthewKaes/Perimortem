// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

namespace Perimortem::System::Compression::BitStream {

// A bit reader for DEFLATE packed streams.
//
// Each byte is consumed starting from its least significant bit, and Huffman
// codes accumulate from the most significant bit to match canonical decode
// order. Both orderings are defined by RFC 1951 and apply regardless of host
// byte order.
class Reader {
 public:
  constexpr Reader(Core::View::Bytes source) : data(source) {}

  auto read_bit() -> Bool;
  auto read_code(Count count) -> Bits_32;
  auto read_raw_bytes(Count count) -> Core::View::Bytes;

  constexpr auto is_valid() const -> Bool {
    return bit_position <= data.get_size() * 8;
  }
  auto invalidate() -> void;

 private:
  Core::View::Bytes data;
  Count bit_position = 0;
};

}  // namespace Perimortem::System::Compression::BitStream
