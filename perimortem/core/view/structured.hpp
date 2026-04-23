// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/utility/func/data.hpp"

#include <tuple>

namespace Perimortem::Core::View {

// A read-only view of continuous data with possible endianness and structure.
template <typename type>
class Structured {
 public:
  Structured() = default;
  Structured(const Structured&) = default;
  Structured(const type* entries, const Count size)
      : source_block(entries), size(size) {}

  constexpr auto contains(const type& data) const -> Bool {
    for (Count i = 0; i < size; i++) {
      if (source_block[i] == data) {
        return true;
      }
    }

    return false;
  }

  constexpr auto at(Count index) const -> const type {
    if (index > size) [[unlikely]] {
      return type();
    }

    return source_block[index];
  }

  constexpr auto operator[](Count index) const -> const type {
    return at(index);
  }

  inline constexpr auto slice(Count start, Count size) const
      -> View::Structured<type> {
    if (start >= get_size())
      return View::Structured<type>();

    const auto size_cap = get_size() - start;
    return View::Structured<type>(source_block + start,
                              size > size_cap ? size_cap : size);
  };

  constexpr auto empty() const -> Bool { return size == 0; };
  constexpr auto get_size() const -> Count { return size; }
  constexpr auto get_data() const -> const type* { return source_block; }

 private:
  const type* source_block = nullptr;
  Count size = 0;
};

}  // namespace Perimortem::Core::View
