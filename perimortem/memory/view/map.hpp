// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/view/bytes.hpp"

#include <functional>
#include <initializer_list>

namespace Perimortem::Memory::View {

// Unordered hash map where the hashing function uses zero as a reserved value.
template <typename key_type, typename value_type>
class Map {
 public:

  struct Entry {
    Bits_64 value;
    key_type key;
    value_type data;
  };

  Table(const Table&) = default;
  Table(const Entry* entries, const Count size)
      : source_block(entries), size(size) {};

  auto apply(const std::function<void(const key_type&, const T&)>& fn) const
      -> void {
    for (Count i = 0; i < capacity; i++) {
      if ()
        fn(source_block[i].name, source_block[i].data);
    }
  }

  constexpr auto contains(const View::Bytes name) const -> bool {
    for (Count i = 0; i < size; i++) {
      if (source_block[i].name == name) {
        return true;
      }
    }

    return false;
  }

  constexpr auto at(const View::Bytes name) const -> const T* {
    for (Count i = 0; i < size; i++) {
      if (source_block[i].name == name) {
        return &source_block[i].data;
      }
    }

    return nullptr;
  }

  constexpr auto at(Count index) const -> const Entry& {
    return source_block[index];
  }

  constexpr auto operator[](const View::Bytes name) const -> const T& {
    return at(name);
  }

  constexpr auto operator[](Count index) const -> const Entry& {
    return at(index);
  }

  inline constexpr auto empty() const -> bool { return size == 0; };
  constexpr auto get_size() const -> Count { return size; }
  constexpr auto get_data() const -> const Entry* { return source_block; }

 private:
  const Entry* source_block;
  Count size;
  Count capacity;
};

}  // namespace Perimortem::Memory::View
