// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/dynamic/bytes.hpp"

namespace Perimortem::System::Compression::BitStream {

// A bit writer for DEFLATE packed streams which dynamically manages a buffer
// compared to the `Access::Bytes` model used by `Core::Writer`.
//
// Each byte is filled starting from its least significant bit, and Huffman
// codes are written from the most significant bit to match canonical decode
// order. Both orderings are defined by RFC 1951 and apply regardless of host
// byte order.
class Writer {
 public:
  Writer(Memory::Dynamic::Bytes& output) : output(output) {}

  auto write_bit(Bool value) -> void;
  auto write_bits(Bits_32 code, Count length) -> void;
  auto write_code(Bits_32 code, Count length) -> void;
  auto flush() -> void;

 private:
  Memory::Dynamic::Bytes& output;
  Count bit_position = 0;
};

}  // namespace Perimortem::System::Compression::BitStream
