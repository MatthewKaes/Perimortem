// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/memory/view/bytes.hpp"
#include "perimortem/core/data_model.hpp"

#include <x86intrin.h>

using namespace Perimortem::Memory;

auto View::Bytes::fast_scan(Byte search, Count position) const -> Count {
  auto search_mask = _mm256_set1_epi8(search);
  const auto value = _mm256_loadu_si256((__m256i*)(source_block + position));
  const auto compare = _mm256_cmpeq_epi8(value, search_mask);
  const Bits_32 offset_mask = _mm256_movemask_epi8(compare);
  return position + __builtin_ctzg(offset_mask);
}

template <Bits_32 channels, Bits_32 index, Bits_32 range>
static auto optimized_or_merge(__m256i source[channels]) -> __m256i {
  if constexpr (range == 1) {
    return source[index];
  } else if constexpr (range == 2) {
    return _mm256_or_si256(source[index], source[index + 1]);
  } else {
    return _mm256_or_si256(
        optimized_or_merge<channels, index, (range / 2)>(source),
        optimized_or_merge<channels, index + (range / 2), range - (range / 2)>(
            source));
  }
}

// Scans a 32 bytes block for the offset of a character.
auto View::Bytes::scan(Byte search, Count position) const -> Count {
  // Use 8 AVX2 channels to get out as much performance as we can for long
  // searches.
  constexpr const auto fused_channels = 8;
  constexpr const auto avx2_channel_width = sizeof(__m256i);
  constexpr const auto full_channel_width = avx2_channel_width * fused_channels;

  auto search_mask = _mm256_set1_epi8(search);
  Count offset = position;
  for (; offset < size; offset += full_channel_width) {
    // Load all channels and check for our target byte.
    __m256i masks[fused_channels];
    for (auto ymm = 0; ymm < fused_channels; ymm++) {
      const auto value = _mm256_loadu_si256(
          (const __m256i*)(source_block + offset + avx2_channel_width * ymm));
      masks[ymm] = _mm256_cmpeq_epi8(value, search_mask);
    }

    // Simple handrolled or optimization for channel depth testing.
    const auto group_mask =
        optimized_or_merge<fused_channels, 0, fused_channels>(masks);

    // Check if we found the byte at all.
    if (!_mm256_testz_si256(group_mask, group_mask)) {
      auto ymm = 0;
      for (auto ymm = 0; ymm < fused_channels - 2; ymm += 2) {
        const Bits_32 result_lower = _mm256_movemask_epi8(masks[ymm]);
        const Bits_32 result_upper = _mm256_movemask_epi8(masks[ymm + 1]);
        const Bits_64 result_merged = (Bits_64)result_upper
                                          << Core::size_in_bits<Bits_32>() |
                                      (Bits_64)result_lower;

        // Since we have additional channels only return if we have our
        // target.
        if (result_merged) {
          return offset + __builtin_ctzg(result_merged) +
                 avx2_channel_width * ymm;
        }
      }

      // We must have a left over register so we can just check it.
      if (ymm == fused_channels - 2) {
        const Bits_32 result_lower = _mm256_movemask_epi8(masks[ymm]);
        const Bits_32 result_upper = _mm256_movemask_epi8(masks[ymm + 1]);
        const Bits_64 result_merged = (Bits_64)result_upper
                                          << Core::size_in_bits<Bits_32>() |
                                      (Bits_64)result_lower;
        return offset + __builtin_ctzg(result_merged) +
               avx2_channel_width * ymm;
      } else {
        const Bits_32 result = _mm256_movemask_epi8(masks[ymm]);
        return offset + __builtin_ctzg(result) + avx2_channel_width * ymm;
      }
    }
  }

  // Scalar fallback to prevent buffer overruns.
  for (; offset < size; offset += 1) {
    if (source_block[position + offset] == search) {
      return offset;
    }
  }

  // Not found.
  return -1;
}
