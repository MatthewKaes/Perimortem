// Perimortem Engine
// Copyright © Matt Kaes

#include "memory/byte_view.hpp"

#include <x86intrin.h>
#include <bit>

using namespace Perimortem::Memory;

template <uint32_t channels, uint32_t index, uint32_t range>
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
auto ByteView::scan(uint8_t search, const uint32_t position) const
    -> uint32_t {
  // Use 8 AVX2 channels to get out as much performance as we can for long
  // searches.
  constexpr const auto fused_channels = 8;
  constexpr const auto avx2_channel_width = sizeof(__m256i);
  constexpr const auto full_channel_width = avx2_channel_width * fused_channels;

  auto search_mask = _mm256_set1_epi8(search);
  uint32_t offset = position;
  for (; offset < size; offset += full_channel_width) {
    // Load all channels and check for our target byte.
    __m256i masks[fused_channels];
    for (auto ymm = 0; ymm < fused_channels; ymm++) {
      const auto value = _mm256_loadu_si256(
          (__m256i*)(rented_block + offset + avx2_channel_width * ymm));
      masks[ymm] = _mm256_cmpeq_epi8(value, search_mask);
    }

    // Simple handrolled or optimization for channel depth testing.
    const auto group_mask =
        optimized_or_merge<fused_channels, 0, fused_channels>(masks);

    // Check if we found the byte at all.
    if (!_mm256_testz_si256(group_mask, group_mask)) {
      auto ymm = 0;
      for (auto ymm = 0; ymm < fused_channels - 2; ymm += 2) {
        const uint32_t result_lower = _mm256_movemask_epi8(masks[ymm]);
        const uint32_t result_upper = _mm256_movemask_epi8(masks[ymm + 1]);
        const uint64_t result_merged =
            (uint64_t)result_upper << 32ul | (uint64_t)result_lower;

        // Since we have additional channels only return if we have our
        // target.
        if (result_merged) {
          return offset + std::countr_zero(result_merged) +
                 avx2_channel_width * ymm;
        }
      }

      // We must have a left over register so we can just check it.
      if (ymm == fused_channels - 2) {
        const uint32_t result_lower = _mm256_movemask_epi8(masks[ymm]);
        const uint32_t result_upper = _mm256_movemask_epi8(masks[ymm + 1]);
        const uint64_t result_merged =
            (uint64_t)result_upper << 32ul | (uint64_t)result_lower;
        return offset + std::countr_zero(result_merged) +
               avx2_channel_width * ymm;
      } else {
        const uint32_t result = _mm256_movemask_epi8(masks[ymm]);
        return offset + std::countr_zero(result) + avx2_channel_width * ymm;
      }
    }
  }

  // Scalar fallback
  for (; offset < size; offset += 1) {
    if (rented_block[position + offset] == '"') {
      return offset;
    }
  }

  // Not found.
  return -1;
}

// Scans ahead 32 bytes to search for value.
auto ByteView::fast_scan(uint8_t search, const uint32_t position) const
    -> uint32_t {
  auto search_mask = _mm256_set1_epi8(search);
  const auto value = _mm256_loadu_si256((__m256i*)(rented_block + position));
  const auto compare = _mm256_cmpeq_epi8(value, search_mask);
  const uint32_t offset_mask = _mm256_movemask_epi8(compare);
  return position + std::countr_zero(offset_mask);
}