// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/uuid.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/system/random.hpp"

using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Perimortem::Core;

#include <x86intrin.h>

auto generate_uuid_v4() -> __m128i {
  __m128i value = _mm_set_epi64x(Random::generate(), Random::generate());

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
constexpr auto convert_to_ascii(__m256i nibbles) -> __m256i {
  const auto numeric_cutoff = _mm256_set1_epi8(0x09);
  const auto ascii_shift = _mm256_set1_epi8('0');
  const auto alpha_values = _mm256_add_epi8(nibbles, ascii_shift);

  const auto ascii_offset = _mm256_set1_epi8('a' - '9' - 1);
  const auto alpha_slots = _mm256_cmpgt_epi8(nibbles, numeric_cutoff);
  const auto slot_offsets = _mm256_and_si256(alpha_slots, ascii_offset);

  const auto final_values = _mm256_add_epi8(alpha_values, slot_offsets);

  return final_values;
}

// Takes a set of nibbles and applies an offset to them to shift them into
// their ascii equivilant value (alphas are shifted to their lower case value).
constexpr auto convert_to_nibble(__m256i ascii) -> __m256i {
  const auto numeric_cutoff = _mm256_set1_epi8('9');
  const auto ascii_shift = _mm256_set1_epi8('0');
  const auto alpha_values = _mm256_sub_epi8(ascii, ascii_shift);

  const auto ascii_offset = _mm256_set1_epi8('a' - '9' - 1);
  const auto alpha_slots = _mm256_cmpgt_epi8(ascii, numeric_cutoff);
  const auto slot_offsets = _mm256_and_si256(alpha_slots, ascii_offset);

  const auto final_values = _mm256_sub_epi8(alpha_values, slot_offsets);

  return final_values;
}

constexpr auto deserialize_ascii(__m256i ascii_buffer,
                                 Static::Vector<Bits_64, 2>& low_high) -> void {
  constexpr SignedBits_8 blank = 0x80;
  const auto nibbles = convert_to_nibble(ascii_buffer);
  auto nibble_high = _mm256_slli_epi16(nibbles, 12);
  auto spaced_bytes = _mm256_or_si256(nibbles, nibble_high);
  const auto packing_shuffle =
      _mm256_set_epi8(blank, blank, blank, blank, blank, blank, blank, blank, 1,
                      3, 5, 7, 9, 11, 13, 15, blank, blank, blank, blank, blank,
                      blank, blank, blank, 1, 3, 5, 7, 9, 11, 13, 15);
  auto packed_bytes = _mm256_shuffle_epi8(spaced_bytes, packing_shuffle);

  low_high[0] = _mm256_extract_epi64(packed_bytes, 2);
  low_high[1] = _mm256_extract_epi64(packed_bytes, 0);
}

auto Uuid::deserialize(const Static::Bytes<36>& uuid_string) -> Uuid& {
  // RFC-4122 spec:
  // 8-4-4-4-12
  const auto buffer = _mm256_loadu_si256(
      reinterpret_cast<const __m256i*>(uuid_string.get_data()));
  const auto offset_buffer = _mm256_loadu_si256(
      reinterpret_cast<const __m256i*>(uuid_string.get_data() + 4));

  const SignedBits_8 blank = 0x80;
  const auto packing_shuffle = _mm256_set_epi8(
      // Lowwer
      blank, blank, blank, blank, 15, 14, 13, 12, 11, 10, 9, 8, 6, 5, 4, 3,
      // Upper
      blank, blank, 15, 14, 12, 11, 10, 9, 7, 6, 5, 4, 3, 2, 1, 0);

  auto packed_bytes = _mm256_shuffle_epi8(buffer, packing_shuffle);
  const auto dash_shuffle = _mm256_set_epi8(
      // Lane 2
      15, 14, 13, 12, blank, blank, blank, blank, blank, blank, blank, blank,
      blank, blank, blank, blank,
      // Lane 1
      13, 12, blank, blank, blank, blank, blank, blank, blank, blank, blank,
      blank, blank, blank, blank, blank);
  auto dash_fill = _mm256_shuffle_epi8(offset_buffer, dash_shuffle);
  auto ascii_buffer = _mm256_or_si256(packed_bytes, dash_fill);

  deserialize_ascii(ascii_buffer, this->low_high);
  return *this;
}

auto Uuid::deserialize(const Static::Bytes<32>& uuid_string) -> Uuid& {
  const auto ascii_buffer = _mm256_loadu_si256(
      reinterpret_cast<const __m256i*>(uuid_string.get_data()));

  deserialize_ascii(ascii_buffer, this->low_high);
  return *this;
}

auto Uuid::serialize() const -> const Static::Bytes<36> {
  Static::Bytes<36> uuid_string;
  auto byte_buffer = uuid_string.get_access().get_data();

  const __m128i packed =
      _mm_loadu_si128(reinterpret_cast<const __m128i*>(&low_high));

  const auto nibbles = nibbler(packed);
  const auto ascii = convert_to_ascii(nibbles);

  // const auto dash_spacing = _mm256_set_epi8();
  const auto dashed = convert_to_ascii(nibbles);

  // Stamp as much data as we can.
  _mm256_storeu_si256(reinterpret_cast<__m256i*>(byte_buffer), dashed);

  // Stamp the dropped ascii into the output.
  // Since shuffle only works on 128 bit lanes for AVX we need to do a stamp for
  // each lane to capture the data that was pushed out.
  auto dropped_2 =
      Data::ensure_endian<Data::ByteOrder::Native, Data::ByteOrder::Big,
                          Bits_16>(_mm256_extract_epi16(ascii, 7));
  auto last_4 =
      Data::ensure_endian<Data::ByteOrder::Native, Data::ByteOrder::Big,
                          Bits_16>(_mm256_extract_epi32(ascii, 7));
  Data::copy(byte_buffer + 16, &dropped_2);
  Data::copy(byte_buffer + 32, &last_4);

  return uuid_string;
}

auto Uuid::generate() -> const Uuid {
  Bits_64 values[2];
  _mm_storeu_si128(reinterpret_cast<__m128i*>(values), generate_uuid_v4());

  return Uuid(values[0], values[1]);
}
