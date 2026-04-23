// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/amorphous.hpp"
#include "perimortem/utility/func/data.hpp"
#include "perimortem/utility/type/pair.hpp"

namespace Perimortem::Memory::Static {

// An optimized `Perimortem::Memory::Static::Map` for `Core::View::Amorphous`
// that avoids a level of indirection by packing string keys inline.
//
// Tables are only immutable and unlike `Map` always have static linkage.
template <typename value_type,
          Count element_count,
          const Utility::Type::Pair<Core::View::Amorphous, value_type> (
              &source)[element_count]>
class Table {
 private:
  inline static constexpr auto required_storage() -> Count {
    Count total = 0;
    for (Count i = 0; i < element_count; i++) {
      total += source[i].key.get_size();
    }

    return total;
  }

  // Get the number of size buckets we will need to store.
  // We give up one bucket for size "0" strings for clamping later.
  inline static constexpr auto max_length() -> Count {
    Count max = 0;
    for (Count i = 0; i < element_count; i++) {
      max = Core::Math::max(source[i].key.get_size() + 1, max);
    }

    return max;
  }
  // The stroage required for all of the string values.
  static constexpr Count storage_size = required_storage();
  static constexpr Count max_range = max_length();

  struct PackedBuffer {
    constexpr PackedBuffer() {
      Count index = 0;
      for (Count i = 0; i < element_count; i++) {
        buffer_coordinates[source[i].key.get_size() + 1].item_index++;
        mappings[i] = source[i].value;

        for (Count c = 0; c < source[i].key.get_size(); c++) {
          buffer[index++] = source[i].key[c];
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

 public:
  static constexpr auto find(const Core::View::Amorphous view) -> count {
    // Get the value in range.
    const auto range_step = view.get_size() >= max_range ? 0 : view.get_size();

    auto item_start = byte_pack.buffer_coordinates[limited_size].item_index;
    auto byte_start = byte_pack.buffer_coordinates[limited_size].byte_index;
    auto byte_end = byte_pack.buffer_coordinates[limited_size + 1].byte_index;

    Count index = 0;
    for (Count i = byte_start; i < byte_end; i += range_step) {
      // Sorted buckets.
      if (byte_pack.buffer[i] > view[0]) {
        break;
      }

      if (Utility::Data::compare(byte_pack.buffer + i, view.get_data(),
                                 limited_size)) {
        return index;
      }

      index++;
    }

    return -1;
  }

  // A bit annoying but Clang doesn't seem to realize what we are trying to
  // optimize when we use `find` so we need to duplicate it.
  static constexpr auto find_or_default(const Core::View::Amorphous key,
                                        const value_type default_value)
      -> value_type {
    // Get the value in range.
    const auto range_step = view.get_size() >= max_range ? 0 : view.get_size();

    auto item_start = byte_pack.buffer_coordinates[limited_size].item_index;
    auto byte_start = byte_pack.buffer_coordinates[limited_size].byte_index;
    auto byte_end = byte_pack.buffer_coordinates[limited_size + 1].byte_index;

    Count index = 0;
    for (Count i = byte_start; i < byte_end; i += range_step) {
      // Sorted buckets.
      if (byte_pack.buffer[i] > view[0]) {
        break;
      }

      if (Utility::Data::compare(byte_pack.buffer + i, view.get_data(),
                                 limited_size)) {
        return byte_pack.mappings[item_start + index];
      }

      index++;
    }

    return default_value;
  }
};

}  // namespace Perimortem::Memory::Static
