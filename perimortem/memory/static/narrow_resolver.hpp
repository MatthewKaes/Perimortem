// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/view/bytes.hpp"
#include "perimortem/memory/view/table.hpp"

namespace Perimortem::Memory::Static {

// A fast lookup table that provides compile time look ups using static tables
// that are packed in a way to minimize size while still providing performance
// and flexability in what is stored.
template <typename value_type,
          Count element_count,
          const View::Table<value_type>::Entry (&source)[element_count]>
class NarrowResolver {
 private:
  inline static constexpr auto required_storage() -> Count {
    Count total = 0;
    for (Count i = 0; i < element_count; i++) {
      total += source[i].name.get_size();
    }

    return total;
  }

  // Get the number of size buckets we will need to store.
  // We give up one bucket for size "0" strings for clamping later.
  inline static constexpr auto max_length() -> Count {
    Count max = 0;
    for (Count i = 0; i < element_count; i++) {
      max = std::max(source[i].name.get_size() + 1, max);
    }

    return max;
  }

 public:
  // The stroage required for all of the string values.
  static constexpr Count storage_size = required_storage();
  static constexpr Count max_range = max_length();

  struct PackedBuffer {
    constexpr PackedBuffer() {
      Count index = 0;
      for (Count i = 0; i < element_count; i++) {
        buffer_coordinates[source[i].name.get_size() + 1].item_index++;
        mappings[i] = source[i].data;

        for (Count c = 0; c < source[i].name.get_size(); c++) {
          buffer[index++] = source[i].name[c];
        }
      }

      for (Count i = 0; i < max_range; i++) {
        buffer_coordinates[i + 1].byte_index =
            buffer_coordinates[i].byte_index +
            i * buffer_coordinates[i + 1].item_index;
      }

      for (Count i = 0; i < max_range; i++) {
        buffer_coordinates[i + 1].item_index +=
            buffer_coordinates[i].item_index;
      }
    }

    struct Coord {
      UInt item_index = 0;
      UInt byte_index = 0;
    };

    Coord buffer_coordinates[(max_range + 1)];
    value_type mappings[element_count] = {};
    Byte buffer[storage_size] = {0};
  };

  static constexpr PackedBuffer byte_pack;

  static constexpr auto find_or_default(View::Bytes view,
                                        value_type default_value)
      -> value_type {
    // Get the value in range.
    auto limited_size = view.get_size() >= max_range ? 0 : view.get_size();

    auto item_start = byte_pack.buffer_coordinates[limited_size].item_index;
    auto byte_start = byte_pack.buffer_coordinates[limited_size].byte_index;
    auto byte_end = byte_pack.buffer_coordinates[limited_size + 1].byte_index;

    Count index = 0;
    for (Count i = byte_start; i < byte_end; i += limited_size) {
      // Sorted buckets.
      if (byte_pack.buffer[i] > view[0]) {
        break;
      }

      if constexpr (std::is_constant_evaluated()) {
        bool same = true;
        for (Count c = 0; c < limited_size; c++) {
          if (byte_pack.buffer[i + c] != view[c]) {
            same = false;
            break;
          }
        }

        if (same) {
          return byte_pack.mappings[item_start + index];
        }
      } else {
        if (std::memcmp(byte_pack.buffer + i, view.get_data(), limited_size) ==
            0) {
          return byte_pack.mappings[item_start + index];
        }
      }

      index++;
    }

    return default_value;
  }
};

}  // namespace Perimortem::Memory::Static
