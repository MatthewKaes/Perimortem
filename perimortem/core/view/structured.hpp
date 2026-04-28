// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/amorphous.hpp"
#include "perimortem/utility/func/data.hpp"

namespace Perimortem::Core::View {

// A read-only view of continuous data with possible endianness and structure.
//
// Structured data can be converted to Amorphous data in order to interperet it
// at a byte level, however this is only valid in memory. To write and read
// binary data as structured data it should be serialized using
// `Perimortem::Serialization::Binary`.
template <typename from>
class Structured {
 public:
  using data_type = from;

  constexpr Structured() = default;
  constexpr Structured(const Structured&) = default;
  constexpr Structured(const data_type* entries, const Count size)
      : source_block(entries), size(size) {}

  // Non-null terminated block of packed bytes.
  template <Count N>
  constexpr Structured(const data_type (&source)[N])
      : source_block(source), size(N) {}

  constexpr operator Amorphous() const {
    return Amorphous(source_block, size * sizeof(data_type));
  }

  constexpr auto contains(const data_type& data) const -> Bool {
    for (Count i = 0; i < size; i++) {
      if (source_block[i] == data) {
        return true;
      }
    }

    return false;
  }

  constexpr auto at(Count index) const -> const data_type {
    if (index > size) [[unlikely]] {
      return data_type();
    }

    return source_block[index];
  }

  constexpr auto operator[](Count index) const -> const data_type {
    return at(index);
  }

  inline constexpr auto slice(Count start, Count size) const
      -> View::Structured<data_type> {
    if (start >= get_size())
      return View::Structured<data_type>();

    const auto size_cap = get_size() - start;
    return View::Structured<data_type>(source_block + start,
                                  size > size_cap ? size_cap : size);
  };

  constexpr auto empty() const -> Bool { return size == 0; };
  constexpr auto get_size() const -> Count { return size; }
  constexpr auto get_data() const -> const data_type* { return source_block; }

 private:
  const data_type* source_block = nullptr;
  Count size = 0;
};

}  // namespace Perimortem::Core::View
