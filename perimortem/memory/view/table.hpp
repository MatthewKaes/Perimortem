// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/view/bytes.hpp"

namespace Perimortem::Memory::View {

// A simple linear look up table for associating managed names to a value.
template <typename T>
class Table {
 public:
  struct Entry {
    constexpr Entry(const View::Bytes name, const T& data) : name(name), data(data) {}
    const View::Bytes name;
    T data;
  };

  Table(const Table&) = default;
  Table(const Entry* entries, const Count size)
      : source_block(entries), size(size) {};


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
};

}  // namespace Perimortem::Memory::View
