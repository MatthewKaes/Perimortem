// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "stack_types.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace Perimortem::Concepts {

template <typename value_type,
          uint64_t element_count,
          const TablePair<const char*, value_type> (&source)[element_count]>
class SparseIndexTable {
  // constexpr goop that needs to be at the top of the class.
 private:
  // Constexpr evaluation to get the size in bytes required to store the longest
  // string in an array. Excludes the null terminator.
  inline static constexpr auto required_storage() -> uint64_t {
    uint64_t max = 0;
    for (uint32_t i = 0; i < element_count; i++) {
      max = std::max(std::char_traits<char>::length(source[i].key), max);
    }
    return max;
  }

  // Constexpr evaluates how many entries should be in the sparse table (data +
  // empty). The number of empty cells is determained by the `sprase_factor`.
  inline static constexpr auto calculate_container_size() -> uint64_t {
    std::array<int, required_storage()> bucket_sizes = {};
    for (uint32_t i = 0; i < element_count; i++) {
      const int index = std::char_traits<char>::length(source[i].key) - 1;
      bucket_sizes[index]++;
    }

    uint64_t max = 0;
    for (uint32_t i = 0; i < required_storage(); i++) {
      max = std::max(std::char_traits<char>::length(source[i].key), max);
    }
    return max + 1;
  }

  using table_layout = std::array<
      TablePair<
          int,
          std::array<TablePair<std::string_view, value_type>,
                     calculate_container_size()>>,
      required_storage()>;

  inline static constexpr auto create_sparse_table() -> const table_layout {
    table_layout table = {};
    for (uint32_t i = 0; i < element_count; i++) {
      const auto& element = source[i];
      uint32_t index = std::char_traits<char>::length(element.key) - 1;
      table[index].value[table[index].key] = {
          std::string_view(element.key, std::char_traits<char>::length(element.key)), element.value};
      table[index].key++;
    }
    return table;
  }

 public:
  // The stroage required for a single key.
  static constexpr uint64_t storage_size = required_storage();
  // The number of blocks in the sparse table.
  static constexpr uint64_t table_size = calculate_container_size();
  // The sprase table containing all of the values as a static array.
  // Mostly useless outside of the class but exposed as it's useful for
  // debugging.
  static constexpr auto sparse_table = create_sparse_table();
  // The number of occupied blocks in the sparse table.
  static constexpr uint64_t item_count = element_count;

  static constexpr auto find_or_default(std::string_view view,
                                        value_type default_value)
      -> value_type {
    if (view.size() == 0 || view.size() > storage_size)
        return default_value;

    const auto& bucket = sparse_table[view.size() - 1];
    for (int i = 0; i < bucket.key; i++) {
      if (bucket.value[i].key[0] != view[0])
        continue;

      if (view.compare(bucket.value[i].key) == 0)
        return bucket.value[i].value;
    }
    return (value_type)(view.size() - 1);
  }
};

}  // namespace Perimortem::Concepts
