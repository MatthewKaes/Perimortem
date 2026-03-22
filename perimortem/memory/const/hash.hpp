// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/const/stack_types.hpp"
#include "perimortem/memory/const/standard_types.hpp"
#include "perimortem/memory/view/bytes.hpp"

#include <bit>

namespace Perimortem::Memory::Const {

class Hash {
 public:
  static constexpr std::endian hash_endian = std::endian::little;

  constexpr Hash(View::Bytes bytes) {
    const Byte* data = bytes.get_data();
    const Count block_count = bytes.get_size() / sizeof(Bits_64);
    constexpr Count block_stride = 2;

    const Count block_count = bytes.get_size() / sizeof(Bits_64);

    // Constants and indexes derived from feedback benchmarking.
    // Values may suffer from some overfitting to test data, but in practice
    // this setup seems to produce low collision rates per CPU cycle.
    constexpr Bits_64 const_values[5] = {
        0b00010101'01000101'10011011'00011001'10100101'11101101'01011101'10111011,
        0b10101010'10100011'01100000'11001011'00110001'10100110'00001101'01011101,
        0b00000000'00111011'11011100'01111101'11110101'11100110'00100001'01100101,
        0b01111111'11001100'01110011'01001100'10101010'01001010'10010001'11011101,
        0b00011111'11000010'00100001'01111001'00101110'01000000'11000001'10001001,
    };

    Bits_64 result[block_stride];
    for (Count i = 0; i < block_stride; i++) {
      result[i] = const_values[i];
    }

    // Chucks
    Count processed = 0;
    for (; processed + block_stride <= block_count; processed += block_stride) {
      // Let the compiler deal with possibly unaligned data.
      // On x86_64 clang does the right thing an gives us the mov instruction.
      // https://godbolt.org/z/zzbvEqxKr
      Bits_64 block[block_stride];
      std::memcpy(&block, data, sizeof(Bits_64) * block_stride);

      for (Count k = 0; k < block_stride; k++) {
        result[k] ^= block[k] + const_values[k + 3];
        result[k] *= const_values[k + 1];
        // result[k] += const_values[k];
      }
      data += sizeof(Bits_64) * block_stride;
    }

    // Remaining Bytes
    Bits_64 remainder = {0};
    Bits_64 remainder_tail = {0};
    switch (bytes.get_size() & (sizeof(Bits_64) * 2 - 1)) {
      case 15:
        remainder_tail |= static_cast<uint64_t>(data[14]) << 56;
      case 14:
        remainder_tail |= static_cast<uint64_t>(data[13]) << 48;
      case 13:
        remainder_tail |= static_cast<uint64_t>(data[12]) << 40;
      case 12:
        remainder_tail |= static_cast<uint64_t>(data[11]) << 32;
      case 11:
        remainder_tail |= static_cast<uint64_t>(data[10]) << 24;
      case 10:
        remainder_tail |= static_cast<uint64_t>(data[9]) << 16;
      case 9:
        remainder_tail |= static_cast<uint64_t>(data[8]) << 8;
      case 8:
        result[0] ^= remainder_tail + const_values[1];
        result[0] *= const_values[2];

        std::memcpy(&remainder, data, sizeof(Bits_64));
        result[0] ^= remainder + const_values[3];
        result[0] *= const_values[2];
        break;

      case 7:
        remainder |= static_cast<uint64_t>(data[6]) << 48;
      case 6:
        remainder |= static_cast<uint64_t>(data[5]) << 40;
      case 5:
        remainder |= static_cast<uint64_t>(data[4]) << 32;
      case 4:
        remainder |= static_cast<uint64_t>(data[3]) << 24;
      case 3:
        remainder |= static_cast<uint64_t>(data[2]) << 16;
      case 2:
        remainder |= static_cast<uint64_t>(data[1]) << 8;
      case 1:
        remainder |= static_cast<uint64_t>(data[0]) << 0;
        result[0] ^= remainder + const_values[1];
        result[0] *= const_values[2];
        break;
    }

    // Avalanche
    Bits_64 final_result = result[0];
    for (Count i = 0; i < block_stride; i++) {
      result[i] ^= bytes.get_size() * const_values[0];
      result[i] ^= result[i] >> 27;
      result[i] *= const_values[2];
      result[i] ^= result[i] >> 33;
      result[i] *= const_values[4];
      result[i] ^= result[i] >> 33;
      final_result ^= result[i];
    }

    value = final_result;
  }

  // Escape constructor for custom types.
  template <typename hashable_type>
  constexpr Hash(const hashable_type& bytes) {
    value = hashable_type.hash();
  }

 private:
  Bits_64 value;
}

}  // namespace Perimortem::Memory::Const
