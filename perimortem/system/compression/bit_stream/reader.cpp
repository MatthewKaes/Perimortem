// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/compression/bit_stream/reader.hpp"

#include "perimortem/core/data.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;

// Loads bytes from the stream into buffer until it buffers at least 57 bits or
// the stream is exhausted. 57 bits guarantees a 9-bit peek after consuming up
// to a 48-bit code + extra-bits pair without a second fill.
auto Compression::BitStream::Reader::fill() -> void {
  const Count size = data.get_size();
  const Bits_8* source_pointer = data.get_data();

  // Fast path: enough bytes remain for a single unaligned 64-bit load.
  // On little-endian hosts (x86) stream byte order matches load order directly.
  //
  // TODO: Support Big endian at some point.
  if (byte_position + Count(sizeof(Bits_64)) <= size) {
    Bits_64 word = *Data::cast<const Bits_64>(source_pointer + byte_position);
    Count consumed = (64 - buffered_bits) >> 3;
    buffer |= word << buffered_bits;
    byte_position += consumed;
    buffered_bits += consumed * 8;
    return;
  }

  // Slow path: near stream end we need to load byte by byte.
  while (buffered_bits <= 56 && byte_position < size) {
    buffer |= Bits_64(source_pointer[byte_position++]) << buffered_bits;
    buffered_bits += 8;
  }
}

auto Compression::BitStream::Reader::read_bit() -> Bool {
  if (buffered_bits == 0) {
    fill();
    if (buffered_bits == 0) [[unlikely]] {
      invalid_stream = True;
      return False;
    }
  }

  // Load the lowest bit of the buffer.
  const Bool bit = buffer & Bits_64(1);
  advance_bits(1);

  return bit;
}

auto Compression::BitStream::Reader::read_code(Count count) -> Bits_32 {
  if (buffered_bits < count) {
    fill();
    if (buffered_bits < count) [[unlikely]] {
      invalid_stream = True;
      return 0;
    }
  }

  const Bits_32 mask = ((Bits_32(1) << count) - Bits_32(1));
  const Bits_32 result = Bits_32(buffer) & mask;
  advance_bits(count);

  return result;
}

auto Compression::BitStream::Reader::read_raw_bytes(Count count)
    -> View::Bytes {
  // Align to byte boundary: discard fractional bits in the current byte.
  const Count fractional_bits = buffered_bits & 7;
  advance_bits(fractional_bits);

  // Rewind any whole bytes pre-loaded into buffer but not yet used.
  byte_position -= buffered_bits >> 3;
  buffer = 0;
  buffered_bits = 0;

  if (byte_position + count > data.get_size()) [[unlikely]] {
    invalid_stream = True;
    return View::Bytes();
  }

  auto result = data.slice(byte_position, count);
  byte_position += count;
  return result;
}

auto Compression::BitStream::Reader::peek_code(Count count) -> Bits_32 {
  if (buffered_bits < count) {
    fill();
  }

  return Bits_32(buffer) & ((Bits_32(1) << count) - Bits_32(1));
}
