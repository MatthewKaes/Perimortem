// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "stack_types.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace Perimortem::Concepts {

//
// Perimortem NarrowResolver
//
/*
  An even faster sparse_index that works as long as the following constraints
  are met:
  - Start of strings are all in a continous range (upper, lower, number, both)
  - No two strings start with the same char.
    - You can work around this by stacking multiple tables.
*/
template <typename value_type,
          uint64_t element_count,
          const TablePair<std::string_view, value_type> (
              &source)[element_count],
          char start_range = 'a',
          char end_range = 'z'>
class NarrowResolver {
 private:
  // Constexpr evaluation to get the size in bytes required to store the longest
  // string in an array. Excludes the null terminator.
  inline static constexpr auto required_storage() -> uint64_t {
    uint64_t max = 0;
    for (uint32_t i = 0; i < element_count; i++) {
      max = std::max(source[i].key.size(), max);
    }

    max--;
    max |= max >> 1;
    max |= max >> 2;
    max |= max >> 4;
    max |= max >> 8;
    max |= max >> 16;
    max++;

    return max;
  }

  template <bool...>
  struct bool_pack;
  template <bool... bs>
  using all_true = std::is_same<bool_pack<bs..., true>, bool_pack<true, bs...>>;

  inline static constexpr auto create_sparse_table()
      -> const std::array<std::array<TablePair<std::string_view, value_type>,
                                     end_range - start_range>,
                          required_storage()> {
    std::array<std::array<TablePair<std::string_view, value_type>,
                          end_range - start_range>,
               required_storage()>
        table = {};
    for (int32_t i = 0; i < element_count; i++) {
      uint32_t index = source[i].key.size() & index_mask;

      // "neat" trick to compile error by causing the function to fail constexpr
      // if we have a duplicate entry. The value of the '-' tells us the index of
      // the element where we encountered the error.
      if (table[index][source[i].key.data()[0] - start_range].key.size() != 0)
        index = source[i].key.data()[-(i + 1)];

      table[index][source[i].key.data()[0] - start_range] = {source[i].key,
                                                             source[i].value};
    }
    return table;
  }

 public:
  // The stroage required for a single key.
  static constexpr uint64_t storage_size = required_storage();
  // Table index mask.
  static constexpr uint64_t index_mask = storage_size - 1;
  // The sprase table containing all of the values as a static array.
  // Mostly useless outside of the class but exposed as it's useful for
  // debugging.
  static constexpr std::array<
      std::array<TablePair<std::string_view, value_type>,
                 end_range - start_range>,
      storage_size>
      sparse_table = create_sparse_table();

  static constexpr auto find_or_default(std::string_view view,
                                        value_type default_value)
      -> value_type {
    uint32_t index = view.size() & index_mask;

    // Seems to optimize more often than the ternary operator, although when
    // properly optimized for x86_64 they end up the same.
    std::array<value_type, 2> options = {
        default_value, sparse_table[index][view[0] - start_range].value};
    return options[sparse_table[index][view[0] - start_range].key == view];
  }
};

}  // namespace Perimortem::Concepts
