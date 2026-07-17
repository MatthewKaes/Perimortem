// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/access/vector.hpp"

namespace Perimortem::Core::Static {

// Typically used for small value type arrays.
template <typename type, Count literal_size>
class Vector {
  template <typename index_type, index_type... indexes>
  struct Indexes {};

  using SourceIndexes = __make_integer_seq<Indexes, Count, literal_size>;

 public:
  static constexpr Count size = literal_size;

  constexpr Vector() {}

  // Static vectors initialize their complete storage from one fixed-size
  // array. Call sites use an extra pair of braces to create that array, as in
  // `Static::Vector<Unsigned_8, 3> values = {{1, 2, 3}}`.
  //
  // Treating the source as one array keeps initialization independent of the
  // number of elements. A variadic constructor would instead manufacture a
  // separate function parameter for every value and make large tables harder
  // for language tooling to analyze.
  constexpr Vector(const type (&source)[literal_size])
      : Vector(source, SourceIndexes()) {}

  // Allows for generating data that would be a pain to manually write out.
  constexpr Vector(type (*generator)(Count)) {
    for (Count i = 0; i < literal_size; i++) {
      source_block[i] = generator(i);
    }
  }

  constexpr operator Core::View::Vector<type>() const { return get_view(); }
  constexpr operator Core::Access::Vector<type>() { return get_access(); }

  constexpr auto operator==(const View::Vector<type>& rhs) -> Bool {
    if (rhs.get_size() != literal_size) {
      return False;
    }

    return Data::compare(source_block, rhs.get_data(), literal_size);
  }

  constexpr auto operator!=(const View::Vector<type>& rhs) -> Bool {
    return !(*this == rhs);
  }

  constexpr auto operator[](Count index) -> type& {
    return source_block[index];
  }

  constexpr auto operator[](Count index) const -> const type& {
    return source_block[index];
  }

  constexpr auto slice(Count start, Count size = Count(-1)) const
      -> Core::View::Vector<type> {
    return get_view().slice(start, size);
  }

  constexpr auto is_empty() const -> Bool { return literal_size == 0; };
  constexpr auto get_size() const -> Count { return literal_size; }
  constexpr auto get_capacity() const -> Count { return literal_size; }
  constexpr auto get_view() const -> const Core::View::Vector<type> {
    return Core::View::Vector<type>(source_block, literal_size);
  }

  constexpr auto get_data() const -> const type* { return source_block; }
  constexpr auto get_data() -> type* { return source_block; }
  constexpr auto get_access() -> Core::Access::Vector<type> {
    return Core::Access::Vector<type>(source_block, literal_size);
  }

 private:
  // C++ cannot copy-construct an array member from another array directly. The
  // private index expansion names each source element in the member initializer
  // so every value is copy-constructed in place. A loop would first require a
  // default value for every element and would then assign over that state.
  template <Count... indexes>
  constexpr Vector(
      const type (&source)[literal_size],
      Indexes<Count, indexes...>)
      : source_block{source[indexes]...} {}

  type source_block[literal_size]{};
};

}  // namespace Perimortem::Core::Static
