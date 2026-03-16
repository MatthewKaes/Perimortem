// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "core/memory/view/bytes.hpp"

#include <functional>

namespace Perimortem::Memory::View {

// A simple linear flat array of trivially constructable values.
template <typename T>
class Vector {
 public:
  Vector(const Vector&) = default;
  Vector(const T* entries, const Count size)
      : source_block(entries), size(size) {}

  auto apply(const std::function<void(const T&)>& fn) const -> void {
    for (Count i = 0; i < size; i++) {
      fn(source_block[i]);
    }
  }

  constexpr auto contains(const T& data) const -> bool {
    for (Count i = 0; i < size; i++) {
      if (source_block[i] == data) {
        return true;
      }
    }

    return false;
  }

  inline constexpr auto at(Count index) const -> const T& {
    return source_block[index];
  }

  inline constexpr auto operator[](Count index) const -> const T& {
    return at(index);
  }

  inline constexpr auto empty() const -> bool { return size == 0; };
  constexpr auto get_size() const -> Count { return size; }
  constexpr auto get_data() const -> const T* { return source_block; }

 private:
  const T* source_block;
  Count size;
};

}  // namespace Perimortem::Memory::View
