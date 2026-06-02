// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/compression/bit_stream/reader.hpp"

#include "perimortem/core/static/bytes.hpp"

using namespace Perimortem::Core;

namespace Perimortem::System::Compression::BitStream {

auto Reader::read_bit() -> Bool {
  // Defines an easy bit mask look up which I think is a bit cleaner in this
  // case than using bit shifts.
  constexpr Static::Bytes<8> bit_masks = {
    0b00000001, 0b00000010, 0b00000100, 0b00001000,
    0b00010000, 0b00100000, 0b01000000, 0b10000000,
  };

  auto logical_bit = bit_position++;
  auto source_byte = data[logical_bit / 8];
  return (source_byte & bit_masks[logical_bit & 0x7]) != 0;
}

auto Reader::read_code(Count count) -> Bits_32 {
  Bits_32 result = 0;
  for (Count i = 0; i < count; i++) {
    result |= Bits_32(read_bit().value) << i;
  }
  return result;
}

auto Reader::read_raw_bytes(Count count) -> View::Bytes {
  bit_position = (bit_position + 7) & ~Count(0x7);

  if (bit_position / 8 + count > data.get_size()) [[unlikely]] {
    invalidate();
    return View::Bytes();
  }

  auto result = data.slice(bit_position / 8, count);
  bit_position += count * 8;
  return result;
}

auto Reader::invalidate() -> void {
  bit_position = data.get_size() * 8 + 1;
}

}  // namespace Perimortem::System::Compression::BitStream
