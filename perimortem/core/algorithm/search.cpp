// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/algorithm/search.hpp"

#include "perimortem/core/static/bytes.hpp"

using namespace Perimortem;
using namespace Perimortem::Core;

#include <x86intrin.h>

namespace Perimortem::Core::Algorithm {

auto load_vector(const Bits_8* data) -> const __m256i {
  return _mm256_loadu_si256(Data::cast<const __m256i_u>(data));
}

auto search(View::Bytes src, View::Bytes value) -> Count {
  // If the value is larger than the source then it can't be a substring.
  if (value.get_size() > src.get_size()) {
    return -1;
  }

  // If the value and src are the same size then we just have to test if they
  // are the same object.
  if (value.get_size() == src.get_size()) {
    return value == src ? 0 : -1;
  }

  // Setup two additional registers with the exact value test as well as the
  // test mask. Since the source can be any length this is easier to setup by
  // loading from two Static::Bytes.
  //
  // Since we can find matches close to the end of the array we need twice the
  // vector size in order to ensure we have valid padding.
  Count i = 0;
  const Count tail_offset = value.get_size() - 1;
  constexpr auto vectorize_limit = sizeof(__m256i);
  if (src.get_size() >= vectorize_limit * 2 &&
      value.get_size() <= vectorize_limit) [[likely]] {
    Static::Bytes<vectorize_limit> value_bytes = value;
    Static::Bytes<vectorize_limit> value_filter;
    Data::set(value_filter.get_data(), 0xFF, value.get_size());
    const auto first_byte = _mm256_set1_epi8(value[0]);
    const auto last_byte = _mm256_set1_epi8(value[tail_offset]);
    const auto test_value = load_vector(value_bytes.get_data());
    const auto test_filter = load_vector(value_filter.get_data());

    // loop through all vectorizable chunks possible.
    // Each chunk tests for 32 possible valid locations based on start and end
    // pairings which performs vastly better than just checking for start values
    // on long ranges.
    for (; i < src.get_size() - vectorize_limit * 2; i += vectorize_limit) {
      const auto head_block =
          _mm256_loadu_si256(Data::cast<const __m256i_u>(src.get_data() + i));
      const auto tail_block = _mm256_loadu_si256(
          Data::cast<const __m256i_u>(src.get_data() + i + tail_offset));

      const auto head_slots = _mm256_cmpeq_epi8(head_block, first_byte);
      const auto tail_slots = _mm256_cmpeq_epi8(tail_block, last_byte);
      const auto test_ranges = _mm256_and_si256(head_slots, tail_slots);
      auto range_mask = Bits_32(_mm256_movemask_epi8(test_ranges));
      while (range_mask) {
        auto index = __builtin_ctzg(range_mask);
        range_mask ^= 1 << index;

        // Load the block and filter it down to only the section we care about.
        const auto test_block = _mm256_loadu_si256(
            Data::cast<const __m256i_u>(src.get_data() + i + index));
        const auto possible_match = _mm256_and_si256(test_block, test_filter);
        const auto match_value = _mm256_cmpeq_epi8(possible_match, test_value);
        auto match = Bits_32(_mm256_movemask_epi8(match_value));
        if (match == 0xFFFFFFFF) {
          return i + index;
        }
      }
    }
  }

  // Scalar fallback using head/tail checking.
  for (; i < src.get_size() - tail_offset; i++) {
    // Skip unless BOTH head and tail bytes match — any mismatch rules out this
    // position without touching the middle bytes.
    if (src[i] != value[0] || src[i + tail_offset] != value[tail_offset]) {
      continue;
    }

    Bool found = True;
    for (Count j = 1; j < tail_offset; j++) {
      if (src[i + j] != value[j]) {
        found = False;
        break;
      }
    }

    if (found) {
      return i;
    }
  }

  // Not found
  return -1;
}

}  // namespace Perimortem::Core::Algorithm
