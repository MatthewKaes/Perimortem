// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/compression/bit_stream/writer.hpp"

#include "perimortem/core/data.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;

// Append complete bytes from the accumulator to the output buffer.
// Leaves at most 7 bits (the partial final byte) in the accumulator.
//
// The accumulator is little-endian so byte 0 is the LSB which maps to
// the bit ordering required by RFC 1951.
//
// TODO: If we ever support ARM we'll need to fix the LSB ordering.
auto Compression::BitStream::Writer::drain() -> void {
  while (bits >= 8) {
    output.append(Bits_8(accumulator & 0xFF));
    accumulator >>= 8;
    bits -= 8;
  }
}

auto Compression::BitStream::Writer::write_bits(Bits_32 value, Count length)
    -> void {
  if (length == 0) {
    return;
  }
  accumulator |= Bits_64(value) << bits;
  bits += length;
  if (bits >= 8) {
    drain();
  }
}

// Huffman codes stored in HuffmanTable are already bit-reversed into stream
// order, so they slot directly into the accumulator without further reversal.
auto Compression::BitStream::Writer::write_code(Bits_32 code, Count length)
    -> void {
  write_bits(code, length);
}

auto Compression::BitStream::Writer::flush() -> void {
  if (bits > 0) {
    output.append(Bits_8(accumulator & 0xFF));
    accumulator = 0;
    bits = 0;
  }
}
