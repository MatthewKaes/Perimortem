// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/amorphous.hpp"
#include "perimortem/core/view/null_terminated.hpp"
#include "perimortem/core/view/structured.hpp"
#include "perimortem/utility/func/data.hpp"
#include "perimortem/utility/type/pair.hpp"

namespace Perimortem::Memory::Static {

// An optimized look up table for `Core::View::Amorphous` that avoids a level of
// indirection by packing string keys inline.
//
// Tables are only immutable and unlike `Map` always have static linkage.
template <typename value_type,
          const Core::View::Structured<
              Utility::Type::Pair<Core::View::Amorphous, value_type>>& source>
class Table {
 private:
  static constexpr auto required_storage() -> Count {
    Count total = 0;
    for (Count i = 0; i < source.get_size(); i++) {
      total += source[i].key.get_size();
    }

    return total;
  }

  // Get the number of size buckets we will need to store.
  // We give up one bucket for size "0" strings for clamping later.
  static constexpr auto max_length() -> Count {
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
    constexpr PackedBuffer() {
      Count index = 0;
      for (Count i = 0; i < source.get_size(); i++) {
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
    value_type mappings[source.get_size()] = {};
    Byte buffer[storage_size] = {0};
  };

  static constexpr PackedBuffer byte_pack;

 public:
  // A bit annoying but Clang doesn't seem to realize what we are trying to
  // optimize when we use `find` so we need to duplicate it.
  static constexpr auto find_or_default(const Core::View::Amorphous key,
                                        const value_type default_value)
      -> value_type {
    // Get the value in range.
    const auto range_step = key.get_size() >= max_range ? 0 : key.get_size();

    auto item_start = byte_pack.buffer_coordinates[range_step].item_index;
    auto byte_start = byte_pack.buffer_coordinates[range_step].byte_index;
    auto byte_end = byte_pack.buffer_coordinates[range_step + 1].byte_index;

    Count index = 0;
    for (Count i = byte_start; i < byte_end; i += range_step) {
      // Sorted buckets.
      if (byte_pack.buffer[i] > key[0]) {
        break;
      }

      if (Utility::Func::Data::compare(byte_pack.buffer + i, key.get_data(),
                                 range_step)) {
        return byte_pack.mappings[item_start + index];
      }

      index++;
    }

    return default_value;
  }

  static constexpr auto get_memory_consumption() const -> Count {
    return sizeof(PackedBuffer);
  }
};

}  // namespace Perimortem::Memory::Static
