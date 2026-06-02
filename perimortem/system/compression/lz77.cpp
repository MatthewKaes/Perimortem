// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/system/compression/lz77.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/math.hpp"

using namespace Perimortem::Core;

namespace Perimortem::System::Compression {

// hash_table maps a 3-byte hash to the most recent input position that
// produced it. chain_table maps each window position (mod window_size) to the
// previous input position with the same hash, forming a singly linked list of
// match candidates. Together they implement a classic LZ77 sliding window
// hash chain.
constexpr Count hash_size = 32768;
constexpr Count window_size = 32768;
constexpr Count window_mask = window_size - 1;
constexpr Bits_32 no_entry = 0xFFFFFFFF;

Lz77::Lz77() {
  hash_table.forgetful_resize(hash_size);
  chain_table.forgetful_resize(window_size);
  reset();
}

auto Lz77::reset() -> void {
  Data::set(
      Data::cast<Byte>(hash_table.get_data()), Byte(0xFF),
      hash_size * sizeof(Bits_32));
  Data::set(
      Data::cast<Byte>(chain_table.get_data()), Byte(0xFF),
      window_size * sizeof(Bits_32));
}

static auto compute_hash(Static::Bytes<3> data) -> Count {
  constexpr Count shift_a = 5;
  constexpr Count shift_b = 10;
  return (Count(data[0]) ^ (Count(data[1]) << shift_a) ^
          (Count(data[2]) << shift_b)) &
         window_mask;
}

auto Lz77::insert(View::Bytes source, Count pos) -> void {
  if (source.get_size() - pos < min_match) {
    return;
  }
  const Byte* at = source.get_data() + pos;
  const Count hash = compute_hash(Static::Bytes<3>{at[0], at[1], at[2]});
  chain_table.get_data()[pos & window_mask] = hash_table.get_data()[hash];
  hash_table.get_data()[hash] = Bits_32(pos);
}

auto Lz77::find_match(View::Bytes source, Count pos, Count depth) const
    -> Match {
  const Byte* data = source.get_data();
  const Count remaining = source.get_size() - pos;

  if (remaining < min_match) {
    return Match();
  }

  const Count hash =
      compute_hash(Static::Bytes<3>{data[pos], data[pos + 1], data[pos + 2]});
  Count best_length = min_match - 1;
  Count best_distance = 0;
  Bits_32 candidate = hash_table.get_data()[hash];

  for (Count d = 0; d < depth && candidate != no_entry; d++) {
    const Count distance = pos - candidate;
    if (distance > window_size) {
      break;
    }

    const Count scan_limit = Math::min(max_match, remaining);
    Count match_length = 0;
    while (match_length < scan_limit &&
           data[pos + match_length] == data[candidate + match_length]) {
      match_length++;
    }

    if (match_length > best_length) {
      best_length = match_length;
      best_distance = distance;
      if (best_length == max_match) {
        break;
      }
    }

    candidate = chain_table.get_data()[candidate & window_mask];
  }

  // No match was found.
  if (best_distance == 0) {
    return Match();
  }
  return Match(best_length, best_distance);
}

}  // namespace Perimortem::System::Compression
