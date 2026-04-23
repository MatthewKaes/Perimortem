// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/amorphous.hpp"
#include "perimortem/utility/func/data.hpp"
#include "perimortem/utility/type/pair.hpp"

namespace Perimortem::Memory::Static {

// A fast lookup table that provides compile time look ups using static tables
// that are packed in a way to minimize size while still providing performance
// and flexability in what is stored.
//
// Keys are assumed to be value types to improve runtime optimizations.
// If keys are backed by a complex type then use a constexpr View wrapper.
//
// Keys are ordered so more frequently accessed keys should be put first.
// If 'sorted' is set then 'operator<' will be used to evaluate early exit.
template <typename key_type,
          typename value_type,
          Count element_count,
          const Utility::Type::Pair<key_type, value_type> (
              &source)[element_count],
          Bool sorted = false>
class Map {
 private:
  static constexpr auto required_storage() -> Count { return element_count; }

  // The stroage required for all of the string values.
  static constexpr Count storage_size = required_storage();

  // Switch packing to SoA (struct of arrays) rahter than AoS for better cache
  // look up on key scan and prevent additional padding from being introduced.
  struct PackedArrays {
    constexpr PackedArrays() {
      for (Count i = 0; i < element_count; i++) {
        keys[i] = source[i].key;
        values[i] = source[i].value;
      }
    }

    key_type keys[element_count] = {};
    value_type values[element_count] = {};
  };

 public:
  constexpr auto find(const key_type key) -> Count {
    for (Count i = 0; i < element_count; i++) {
      if (packed.keys[i] == key) {
        return i;
      }

      if constexpr (sorted) {
        if (packed.keys[i] > key) {
          return Count(-1);
        }
      }
    }

    return Count(-1);
  }

  constexpr auto find_or_default(Core::View::Amorphous view,
                                 value_type default_value) -> value_type {
    const auto index = find(view);
    if (index == Count(-1)) {
      return default_value;
    }

    return packed.values[index];
  }

  constexpr auto contains(const key_type& key) const -> Bool {
    return find(key) != Count(-1);
  }

  constexpr auto at(const key_type& key) -> value_type& {
    auto index = find(key);
    if (index == Count(-1)) {
      return null;
    }

    // Return end block
    return packed.values[index];
  }

  constexpr auto operator[](const key_type& key) -> value_type& {
    return at(key);
  }

  constexpr auto get_size() const -> Count { return element_count; }

 private:
  PackedArrays packed = {};
  value_type null = {};
};

}  // namespace Perimortem::Memory::Static
