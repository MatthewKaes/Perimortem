// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include <cstdint>
#include <functional>

namespace Perimortem::Memory::View {

// A simple linear flat array of trivially constructable values.
template <typename T>
class Vector {
 public:
  Vector(const Vector&) = default;
  Vector(const T* entries, const uint64_t size)
      : source_block(entries), size(size) {}

  auto apply(const std::function<void(const T&)>& fn) const -> void {
    for (uint64_t i = 0; i < size; i++) {
      fn(source_block[i]);
    }
  }

  constexpr auto contains(const T& data) const -> bool {
    for (uint64_t i = 0; i < size; i++) {
      if (source_block[i] == data) {
        return true;
      }
    }

    return false;
  }

  inline constexpr auto at(uint64_t index) const -> const T& {
    return source_block[index];
  }

  inline constexpr auto operator[](uint64_t index) const -> const T& {
    return at(index);
  }

  constexpr auto get_size() const -> uint64_t { return size; }
  constexpr auto get_data() const -> const T* { return source_block; }

 private:
  const T* source_block;
  uint64_t size;
};

}  // namespace Perimortem::Memory::View
