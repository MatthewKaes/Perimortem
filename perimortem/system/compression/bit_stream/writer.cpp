// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/compression/bit_stream/writer.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/data.hpp"

using namespace Perimortem::Core;

namespace Perimortem::System::Compression::BitStream {

auto Writer::write_bit(Bool value) -> void {
  constexpr Static::Bytes<8> bit_masks = {
    0b00000001, 0b00000010, 0b00000100, 0b00001000,
    0b00010000, 0b00100000, 0b01000000, 0b10000000,
  };

  auto logical_bit = bit_position++;
  if ((logical_bit & 0x7) == 0) {
    output.append(0x00);
  }

  if (value) {
    output.get_access().get_data()[output.get_size() - 1] |=
        bit_masks[logical_bit & 0x7];
  }
}

auto Writer::write_bits(Bits_32 code, Count length) -> void {
  for (Count i = 0; i < length; i++) {
    write_bit(Bool((code >> i) & 1));
  }
}

auto Writer::write_code(Bits_32 code, Count length) -> void {
  for (Count i = length; i > 0; i--) {
    write_bit(Bool((code >> (i - 1)) & 1));
  }
}

auto Writer::flush() -> void {
  Data::align<8>(bit_position);
}

}  // namespace Perimortem::System::Compression::BitStream
