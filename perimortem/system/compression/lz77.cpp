// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/compression/lz77.hpp"

#include <x86intrin.h>

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/math.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;

// `hash_table` maps a 3-byte hash to the most recent input position that
// produced it. `chain_table` maps each window position to the previous input
// position with the same hash, forming a linked list of match candidates.
// Together they implement a classic LZ77 sliding window hash chain.
constexpr Count hash_size = 32768;
constexpr Count window_size = 32768;
constexpr Count window_mask = window_size - 1;
constexpr Bits_32 null_entry = 0xFFFFFFFF;

constexpr auto compute_hash(Static::Bytes<3> data) -> Count {
  // Knuth multiplicative hash (2^32 / golden ratio) is fast and provides a
  // better distribution for the has table.
  constexpr Bits_32 knuth_multiplier = 2654435761;
  Bits_32 v =
      (Bits_32(data[0]) << 16) | (Bits_32(data[1]) << 8) | Bits_32(data[2]);
  return Count((v * knuth_multiplier) >> 17);
}

// Compare source[position..] against source[candidate..] up to scan_limit bytes,
// looking for instances of duplication in earlier parts of the source input.
constexpr auto extend_match(
    const Bits_8* data,
    Count position,
    Count candidate,
    Count scan_limit) -> Count {
  // Do a vectorized scan until we find at least one byte that doesn't match.
  // Unrolling the loop fully causes short lengths to suffer. Two checks fit
  // nicely in a single 64 bit mask emulating AVX512 so we go with that.
  Count match_length = 0;
  constexpr auto fused_channels = 2;
  constexpr auto avx2_channel_width = sizeof(__m256i);
  constexpr auto full_channel_width = avx2_channel_width * fused_channels;
  while (match_length + full_channel_width <= scan_limit) {
    // Load the lower bytes
    const auto lower_source_chunk = _mm256_loadu_si256(
        Data::cast<const __m256i>(data + position + match_length));
    const auto lower_candidate_chunk = _mm256_loadu_si256(
        Data::cast<const __m256i>(data + candidate + match_length));

    // Load the upper bytes
    const auto upper_source_chunk = _mm256_loadu_si256(
        Data::cast<const __m256i>(
            data + position + match_length + avx2_channel_width));
    const auto upper_candidate_chunk = _mm256_loadu_si256(
        Data::cast<const __m256i>(
            data + candidate + match_length + avx2_channel_width));

    // Perform two parallel compares and merge the masks on the data dependency.
    const Bits_64 mismatch_mask =
        ~Bits_64(_mm256_movemask_epi8(
            _mm256_cmpeq_epi8(lower_source_chunk, lower_candidate_chunk))) |
        (~Bits_64(_mm256_movemask_epi8(
             _mm256_cmpeq_epi8(upper_source_chunk, upper_candidate_chunk)))
         << 32);

    // Scans 64 bytes to find a mismatch.
    if (mismatch_mask != 0) {
      match_length += Count(__builtin_ctzll(mismatch_mask));
      return match_length;
    }

    match_length += full_channel_width;
  }

  // First deal with the residual in 8 byte chunks.
  constexpr Count word_size = Count(sizeof(Bits_64));
  while (match_length + word_size <= scan_limit) {
    Bits_64 source_chunk = *Data::cast<Bits_64>(data + position + match_length);
    Bits_64 candidate_chunk =
        *Data::cast<Bits_64>(data + candidate + match_length);

    if (source_chunk != candidate_chunk) {
      match_length +=
          Count(__builtin_ctzll(source_chunk ^ candidate_chunk)) / 8;
      return match_length;
    }

    match_length += word_size;
  }

  // Final scalar loop for up to 7 residual bytes.
  while (match_length < scan_limit &&
         data[position + match_length] == data[candidate + match_length]) {
    match_length++;
  }
  return match_length;
}

Compression::Lz77::Lz77() {
  hash_table.forgetful_resize(hash_size);
  chain_table.forgetful_resize(window_size);
  reset();
}

auto Compression::Lz77::reset() -> void {
  Data::set(
      Data::cast<Bits_8>(hash_table.get_data()), Bits_8(0xFF),
      hash_size * sizeof(Bits_32));
  Data::set(
      Data::cast<Bits_8>(chain_table.get_data()), Bits_8(0xFF),
      window_size * sizeof(Bits_32));
}

auto Compression::Lz77::insert(View::Bytes source, Count position) -> void {
  if (source.get_size() - position < min_match) {
    return;
  }
  const Count hash =
      compute_hash(Static::Bytes<3>::read_range(source.get_data() + position));
  chain_table.get_data()[position & window_mask] = hash_table.get_data()[hash];
  hash_table.get_data()[hash] = Bits_32(position);
}

auto Compression::Lz77::find_match_and_insert(
    View::Bytes source,
    Count position,
    Count depth) -> Match {
  const Bits_8* data = source.get_data();
  const Count remaining = source.get_size() - position;

  // Only perform a match if we have enough bytes for it to be worthwhile.
  if (remaining < min_match) {
    return Match();
  }

  // Insert position before searching so future calls can find it but start the
  // search at the old chain head so we never match position against itself.
  const Count hash = compute_hash(Static::Bytes<3>::read_range(data + position));
  Bits_32 candidate = hash_table.get_data()[hash];
  chain_table.get_data()[position & window_mask] = candidate;
  hash_table.get_data()[hash] = Bits_32(position);

  Count best_length = min_match - 1;
  Count best_distance = 0;
  const Count scan_limit = Math::min(max_match, remaining);

  for (Count i = 0; i < depth && candidate != null_entry; i++) {
    const Count distance = position - candidate;
    if (distance > window_size) {
      break;
    }

    Count match_length = extend_match(data, position, Count(candidate), scan_limit);
    if (match_length > best_length) {
      best_length = match_length;
      best_distance = distance;
      if (best_length == max_match) {
        break;
      }
    }

    // Select the candidate to see if we can find an improved match.
    candidate = chain_table.get_data()[candidate & window_mask];
  }

  if (best_distance == 0) {
    return Match();
  }
  return Match(best_length, best_distance);
}

auto Compression::Lz77::find_match(View::Bytes source, Count position, Count depth)
    const -> Match {
  const Bits_8* data = source.get_data();
  const Count remaining = source.get_size() - position;

  // Only perform a match if we have enough bytes for it to be worthwhile.
  if (remaining < min_match) {
    return Match();
  }

  const Count hash = compute_hash(Static::Bytes<3>::read_range(data + position));
  Count best_length = min_match - 1;
  Count best_distance = 0;
  Bits_32 candidate = hash_table.get_data()[hash];
  const Count scan_limit = Math::min(max_match, remaining);

  for (Count i = 0; i < depth && candidate != null_entry; i++) {
    const Count distance = position - candidate;
    if (distance > window_size) {
      break;
    }

    const Count match_length =
        extend_match(data, position, Count(candidate), scan_limit);
    if (match_length > best_length) {
      best_length = match_length;
      best_distance = distance;

      // If we can't find a match then just bail out.
      if (best_length == max_match) {
        break;
      }
    }

    candidate = chain_table.get_data()[candidate & window_mask];
  }

  // If we couldn't find a match then just return a null match.
  if (best_distance == 0) {
    return Match();
  }
  return Match(best_length, best_distance);
}
