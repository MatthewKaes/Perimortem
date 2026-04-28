// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"
#include "perimortem/core/access/amorphous.hpp"

namespace Perimortem::Core::Access {

// A read-write view of continuous data with possible endianness and structure.
//
// Structured data can be converted to Amorphous data in order to interperet it
// at a byte level, however this is only valid in memory. To write and read
// binary data as structured data it should be serialized using
// `Perimortem::Serialization::Binary`.
template <typename form>
class Structured {
 public:
  using data_type = form;

  // Default to empty string.
  constexpr Structured() : source_block(nullptr), size(0) {}

  constexpr Structured(const Structured&) = default;

  constexpr Structured(data_type* source, Count source_size)
      : source_block(source), size(source_size) {}

  constexpr operator Amorphous() const {
    return Amorphous(source_block, size * sizeof(type));
  }

  inline constexpr auto empty() const -> Bool { return size == 0; };
  inline constexpr auto get_size() const -> Count { return size; };
  inline constexpr auto get_data() -> data_type* { return source_block; };

  constexpr auto at(Count index) -> type& {
    if (index > size) [[unlikely]] {
      static type oob;
      return oob;
    }

    return source_block[index];
  }

  constexpr auto operator[](Count index) -> type& { return at(index); }

  inline constexpr auto slice(Count start, Count size) const
      -> Access::Structured<type> {
    if (start >= get_size())
      return Access::Structured<type>();

    const auto size_cap = get_size() - start;
    return Access::Structured<type>(source_block + start,
                                    size > size_cap ? size_cap : size);
  };

 private:
  data_type* source_block;
  Count size;
};

}  // namespace Perimortem::Core::Access
