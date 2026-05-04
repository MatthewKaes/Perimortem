// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/data.hpp"
#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/utility/pair.hpp"

namespace Perimortem::Utility {

// An optimized look up table for `Core::View::Bytes` that avoids a level of
// indirection by packing string keys inline.
//
// Tables are only immutable and unlike `Map` always have static linkage.
//
// The source template parameter is a bit funky since the type itself is
// parameterized over it's source data to create a type that expresses the
// compressed form of the lookups.
template <typename value_type,
          const Core::View::Vector<
              Utility::Pair<Core::View::Bytes, value_type>>& source,
          Core::Data::CacheAware cache_aware = Core::Data::CacheAware::Enabled>
class Table {
 public:
  static constexpr auto cache_line_size = static_cast<Bits_64>(cache_aware);
  static consteval auto required_storage() -> Count {
    Count buckets[max_length()] = {0};

    for (Count i = 0; i < source.get_size(); i++) {
      buckets[source[i].key.get_size()] += source[i].key.get_size();
    }

    Count total = 0;
    for (Count i = 0; i < max_length(); i++) {
      const auto cache_alignment = (total % cache_line_size);
      if (cache_alignment != 0 &&
          cache_alignment + buckets[i] > cache_line_size) {
        total += cache_line_size - cache_alignment;
      }

      total += buckets[i];
    }
    return total;
  }

  // Get the number of size buckets we will need to store.
  // We give up one bucket for size "0" strings for clamping later.
  static consteval auto max_length() -> Count {
    Count max = 0;
    for (Count i = 0; i < source.get_size(); i++) {
      max = max > source[i].key.get_size() + 1 ? max
                                               : source[i].key.get_size() + 1;
    }

    return max;
  }
  // The stroage required for all of the string values.
  static constexpr Count storage_size = required_storage();
  static constexpr Count max_range = max_length();

  struct PackedBuffer {
    consteval PackedBuffer() {
      // Caculate the bytes required for each bucket
      Count buckets[max_length()] = {0};
      for (Count i = 0; i < source.get_size(); i++) {
        buckets[source[i].key.get_size()] += source[i].key.get_size();
      }

      // Align and set all bucket indexes
      Count total = 0;
      for (Count i = 0; i < max_length(); i++) {
        // If the bucket crosses a cache line then move it over.
        const auto cache_alignment = (total % cache_line_size);
        if (cache_alignment != 0 &&
            cache_alignment + buckets[i] > cache_line_size) {
          total += cache_line_size - cache_alignment;
        }

        buffer_coordinates[i].byte_index = total;
        total += buckets[i];
        buckets[i] = buffer_coordinates[i].byte_index;
      }

      total = 0;
      for (Count size = 0; size < max_length(); size++) {
        buffer_coordinates[size].item_index = total;

        for (Count i = 0; i < source.get_size(); i++) {
          if (source[i].key.get_size() != size) {
            continue;
          }

          mappings[total] = source[i].value;
          buffer_coordinates[size].item_count++;
          total++;

          for (Count c = 0; c < source[i].key.get_size(); c++) {
            buffer[buckets[size]++] = source[i].key[c];
          }
        }
      }
    }

    struct Coord {
      Bits_8 item_index = 0;
      Bits_8 item_count = 0;
      Bits_16 byte_index = 0;
    };

    Coord buffer_coordinates[(max_range + 1)];
    value_type mappings[source.get_size()] = {};
    alignas(64) Byte buffer[storage_size] = {0};
  };

  static constexpr PackedBuffer byte_pack;

 public:
  static constexpr auto find_or_default(const Core::View::Bytes key,
                                        const value_type default_value)
      -> value_type {
    // Get the value in range.
    const auto range_step = key.get_size();
    if (range_step > max_range) {
      return default_value;
    }

    auto item_start = byte_pack.buffer_coordinates[range_step].item_index;
    auto item_count = byte_pack.buffer_coordinates[range_step].item_count;
    auto byte_start = byte_pack.buffer_coordinates[range_step].byte_index;

    for (Bits_8 i = 0; i < item_count; i++) {
      const auto byte_index = byte_start + i * range_step;
      if (byte_pack.buffer[byte_index] != key[0]) {
        continue;
      }

      if (Core::Data::compare(byte_pack.buffer + byte_index, key.get_data(),
                              range_step)) {
        return byte_pack.mappings[item_start + i];
      }
    }

    return default_value;
  }

  static consteval auto get_memory_consumption() -> Count {
    return sizeof(PackedBuffer);
  }
};

}  // namespace Perimortem::Utility
