// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/uuid.hpp"
#include "perimortem/system/random.hpp"

using namespace Perimortem::Memory;
using namespace Perimortem::System;

#include <x86intrin.h>

auto generate_uuid_v4() -> __m128i {
  __m128i value =
    _mm_set_epi64x(Random::generate(), Random::generate());

  const auto v4_mask =
      _mm_set_epi64x(0xFFFFFFFFFFFFFF3Full, 0xFF0FFFFFFFFFFFFFull);
  const auto v4_set =
      _mm_set_epi64x(0x0000000000000080ull, 0x0040000000000000ull);

  return _mm_or_si128(_mm_and_si128(value, v4_mask), v4_set);
}

// Expand 128 bits into nibbles spread across 256 bits.
// Bytes are packed from:
// [hhhhllll][hhhhllll][hhhhllll][hhhhllll]...
// [____hhhh][____llll][____hhhh][____llll]...
constexpr auto nibbler(__m128i packed_guid) -> __m256i {
  const auto nibble_mask = _mm256_set1_epi8(0x0F);
  auto nibble_high = _mm_srli_epi64(packed_guid, 4);
  auto two_byte_pack = _mm256_and_si256(
      _mm256_set_m128i(_mm_unpackhi_epi8(nibble_high, packed_guid),
                       _mm_unpacklo_epi8(nibble_high, packed_guid)),
      nibble_mask);

  return two_byte_pack;
}

// Takes a set of nibbles and applies an offset to them to shift them into
// their ascii equivilant value (alphas are shifted to their lower case value).
constexpr auto convert_to_ascii(__m256i nibbles) {
  const auto numeric_cutoff = _mm256_set1_epi8(0x09);
  const auto ascii_shift = _mm256_set1_epi8('0');
  const auto alpha_values = _mm256_add_epi8(nibbles, ascii_shift);

  const auto ascii_offset = _mm256_set1_epi8('a' - '9' - 1);
  const auto alpha_slots = _mm256_cmpgt_epi8(nibbles, numeric_cutoff);
  const auto slot_offsets = _mm256_and_si256(alpha_slots, ascii_offset);

  const auto final_values = _mm256_add_epi8(alpha_values, slot_offsets);

  return final_values;
}

auto Uuid::deserialize(const Static::Bytes<36>& uuid_string) -> Uuid& {
  // RFC-4122 spec:
  // 8-4-4-4-12

  return *this;
}

auto Uuid::serialize() const -> Static::Bytes<36> {
  Static::Bytes<36> uuid_string;
  // _mm256_storeu_si256(reinterpret_cast<__m256i*>(uuid_string.get_data()),
  //                     nibble_value);
  // Bits_64 upper = (value[0] >> 4) & 0x0F0F0F0F0F0F0F0FULL;
  // Bits_64 lower = value[0] & 0x0F0F0F0F0F0F0F0FULL;

  return uuid_string;
}

auto Uuid::generate() -> Uuid {
  Bits_64 values[2];
  _mm_storeu_si128(reinterpret_cast<__m128i*>(values), generate_uuid_v4());

  return Uuid(values);
}
