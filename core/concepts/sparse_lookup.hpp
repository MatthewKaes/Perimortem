// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "stack_types.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace Perimortem::Concepts {
//
// Perimortem SparseLookupTable
//
/*
  A generalized solution for compile time & runtime optimized value lookups
  based on compiler time strings. The table itself performs no allocations and
  performs only constexpr operations whenever possible allowing for several
  compile time optimizations you can't get with std::unordered_map or any
  variant there of.

  The tables are optimized to do as few checks as possible but requires some
  tunning for optimal results. The table size and seed can be tuned and if
  `has_perfect_hash` is true then the table _guarantees_ the following
  comparision count performance:

  Invalid key:
  best        -> 1
  worst       -> sizeof(key) + 1
  runtime key -> sizeof(key) * 2 + 1

  Valid key:
  best        -> sizeof(key) + 1
  worst       -> sizeof(key) + 1
  runtime key -> sizeof(key) * 2 + 1

  For runtime keys (e.g.: std::string_view) it is typically slower to convert it
  into a const size format than to perform the sub optimal query. If you are
  building the strings for table checks, StackString can help optimize calls.
  It also helps if you will be using the value over multiple queries.

  Perfect hashing can be check via a static_assert to catch performance
  regressions as new entries are added.
*/
template <typename value_type,
          uint64_t element_count,
          const TablePair<const char*, value_type> (&source)[element_count],
          uint64_t sparse_factor = 2'00,
          uint64_t sparse_seed = 0x506572696d6f7274,
          bool use_radix_hashing = true>
class SparseLookupTable {
  // constexpr goop that needs to be at the top of the class.
 private:
  // Constexpr hash with speedy performance.
  // Based on the FNV hash (http://isthe.com/chongo/tech/comp/fnv/) but with a
  // few tradeoffs to optimize for speed. This can be done because we assume the
  // user will actually tweak controls at template instantiation time to aim for
  // perfect hashes.
  inline static constexpr uint64_t hash_string(const char* str,
                                               uint64_t length) {
    constexpr auto initial_state = (0xcbf29ce484222325 ^ sparse_seed) *
                                   static_cast<uint64_t>(0x100000001b3ull);
    if (length == 0)
      return initial_state;

    uint64_t d = initial_state;
    if (use_radix_hashing) {
      uint8_t step_count = radix_stride(length);
      switch (step_count) {
        case 1: {
          for (uint32_t i = 0; i < length; i++)
            d = (d ^ static_cast<uint64_t>(str[i])) *
                static_cast<uint64_t>(0x100000001b3ull);
          return d;
        }
        case 2: {
          for (uint32_t i = 0; i < length; i += 2)
            d = (d ^ static_cast<uint64_t>(str[i] ^ str[i + 1])) *
                static_cast<uint64_t>(0x100000001b3ull);
          return d;
        }
        case 4: {
          for (uint32_t i = 0; i < length; i += 4)
            d = (d ^ static_cast<uint64_t>(str[i] ^ str[i + 1] ^ str[i + 2] ^
                                           str[i + 3])) *
                static_cast<uint64_t>(0x100000001b3ull);
          return d;
        }
        case 8: {
          for (uint32_t i = 0; i < length; i += 4)
            d = (d ^ static_cast<uint64_t>(
                         str[i] ^ str[i + 1] ^ str[i + 2] ^ str[i + 3] ^
                         str[i + 4] ^ str[i + 5] ^ str[i + 6] ^ str[i + 7])) *
                static_cast<uint64_t>(0x100000001b3ull);
          return d;
        }
      }
    } else {
      for (uint32_t i = 0; i < length; i++)
        d = (d ^ static_cast<uint64_t>(str[i])) *
            static_cast<uint64_t>(0x100000001b3ull);
      return d;
    }
  }

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
  inline static constexpr auto calculate_container_size(uint64_t value)
      -> uint64_t {
    // Assume we need padding to avoid collisions and you'll never use the
    // full space or you'll have other problems. Floats can't be used as
    // template parameters so to have finer grain control we use fix point.
    return value * sparse_factor / 100;
  }

  inline static constexpr auto create_sparse_table() -> const
      std::array<TablePair<StackString<required_storage()>, value_type>,
                 calculate_container_size(element_count)> {
    std::array<TablePair<StackString<required_storage()>, value_type>,
               calculate_container_size(element_count)>
        table = {};
    for (uint32_t i = 0; i < element_count; i++) {
      const auto& element = source[i];
      uint32_t hash =
          hash_string(element.key, std::char_traits<char>::length(element.key));
      for (uint32_t check = 0; check < element_count; check++) {
        if (table[(hash + check) % calculate_container_size(element_count)]
                .key.size() == 0) {
          table[(hash + check) % calculate_container_size(element_count)] = {
              StackString<required_storage()>(element.key), element.value};
          break;
        }
      }
    }
    return table;
  }

 public:
  // The stroage required for a single key.
  static constexpr uint64_t storage_size = required_storage();
  // The number of blocks in the sparse table.
  static constexpr uint64_t table_size =
      calculate_container_size(element_count);
  // The sprase table containing all of the values as a static array.
  // Mostly useless outside of the class but exposed as it's useful for
  // debugging.
  static constexpr auto sparse_table = create_sparse_table();

  static constexpr auto find_or_default(const char* data,
                                        value_type default_value)
      -> value_type {
    uint64_t size = std::char_traits<char>::length(data);
    return find_or_default(data, size, default_value);
  }

  static constexpr auto find_or_default(const char* data,
                                        uint64_t size,
                                        value_type default_value)
      -> value_type {
    // Check if the table has any collisions or if it's perfect.
    constexpr bool no_collisions = has_perfect_hash(1);
    uint32_t hash = hash_string(data, size);

    // Optimize for tables when we know we won't have to perform a
    // table walk.
    if (no_collisions) {
      const auto& entry = sparse_table[hash % table_size];
      if (entry.key.size() != size)
        return default_value;

      for (uint64_t i = 0; i < size; i++) {
        if (entry.key.data()[i] != data[i])
          return default_value;
      }

      return entry.value;
    } else {
      for (uint32_t check = 0; check < max_checks(); check++) {
        const auto& entry = sparse_table[(hash + check) % table_size];
        if (entry.key.size() == 0)
          return default_value;

        if (entry.key.size() != size)
          continue;

        for (uint64_t i = 0; i < size; i++) {
          if (entry.key.data()[i] != data[i])
            continue;
        }

        return entry.value;
      }

      return default_value;
    }
  }

  // Gets the worst case number of checks that need to be performed
  // in order to resolve a lookup.
  //
  // If max_checks() is greater than 1 (hash is not perfect), then
  // the performance of check operations will be quite a bit slower,
  // even for other cases that only require 1 check.
  static constexpr auto max_checks() -> uint64_t {
    uint64_t max = 0;
    for (auto count : check_counts()) {
      max = std::max(max, count);
    }
    return max;
  }

  static constexpr auto has_perfect_hash(uint64_t threshold = 1) -> bool {
    return max_checks() <= threshold;
  }

  static constexpr auto has_broken_hash() -> bool { return max_checks() == -1; }

 private:
  static constexpr auto check_counts() -> std::array<uint64_t, element_count> {
    std::array<uint64_t, element_count> counts = {};
    for (uint32_t i = 0; i < element_count; i++) {
      counts[i] = check_count(source[i].key);
    }
    return counts;
  }

  static constexpr auto check_count(const char* data) -> value_type {
    uint64_t size = std::char_traits<char>::length(data);
    return check_count(data, size);
  }

  static constexpr auto check_count(const char* data, uint64_t size)
      -> uint64_t {
    uint32_t hash = hash_string(data, size);
    for (uint32_t check = 0; check < element_count; check++) {
      const auto& entry = sparse_table[(hash + check) % table_size];
      if (entry.key.size() == 0)
        return -1;

      if (entry.key.size() != size)
        continue;

      for (uint64_t i = 0; i < size; i++) {
        if (entry.key.data()[i] != data[i])
          continue;
      }

      return check + 1;
    }

    return element_count;
  }

  // Make sure we don't have a broken hash.
  static_assert(has_broken_hash() == false);
};

struct optimial_result {
  bool valid = false;
  uint64_t seed = 0;
  uint64_t size_factor = 100;
};

// SeedFinder uses template meta programming to find a template that meets the
// required collision constraints. By default it searches for perfect seeds.
template <uint64_t search_depth,
          typename value_type,
          uint64_t element_count,
          const TablePair<const char*, value_type> (&source)[element_count],
          uint64_t sparse_factor = 2'00,
          uint64_t sparse_seed = 0x506572696d6f7274,
          bool use_radix_hashing = true,
          uint32_t collision_threshold = 1>
class SeedFinder {
 public:
  static_assert(false,
                "SeedFinder should never be instantiated as part of the build. "
                "Make sure to remove before building as it massively slows "
                "down build time.");
  static_assert(search_depth <= 255,
                "Search depth is greater than maximum allowed depth of 255.");

  inline static constexpr auto candidate_search() -> uint64_t {
    constexpr SparseLookupTable<value_type, element_count, source,
                                sparse_factor, sparse_seed + search_depth,
                                use_radix_hashing>
        table;
    if (table.has_perfect_hash(collision_threshold)) {
      return sparse_seed + search_depth;
    }

    return SeedFinder<search_depth - 1, value_type, element_count, source,
                      sparse_factor, sparse_seed, use_radix_hashing,
                      collision_threshold>::candidate_search();
  }
};

// Terminal state if we can't find a seed.
template <typename value_type,
          uint64_t element_count,
          const TablePair<const char*, value_type> (&source)[element_count],
          uint64_t sparse_factor,
          uint64_t sparse_seed,
          bool use_radix_hashing,
          uint32_t collision_threshold>
class SeedFinder<0,
                 value_type,
                 element_count,
                 source,
                 sparse_factor,
                 sparse_seed,
                 use_radix_hashing,
                 collision_threshold> {
 public:
  inline static constexpr auto candidate_search() -> uint64_t { return 0; }
};

}  // namespace Perimortem::Concepts
