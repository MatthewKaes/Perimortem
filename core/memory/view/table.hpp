// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "memory/view/bytes.hpp"

#include <functional>
#include <initializer_list>

namespace Perimortem::Memory::View {

// A simple linear look up table for associating managed names to a value.
template <typename T>
class Table {
 public:
  struct Entry {
    Entry(const View::Bytes name, const T& data) : name(name), data(data) {}
    const View::Bytes name;
    T data;
  };

  Table(const Table&) = default;
  Table(const Entry* entries, const uint64_t size)
      : source_block(entries), size(size) {};

  auto apply(const std::function<void(const View::Bytes, const T&)>& fn) const
      -> void {
    for (uint64_t i = 0; i < size; i++) {
      fn(source_block[i].name, source_block[i].data);
    }
  }

  constexpr auto contains(const View::Bytes name) const -> bool {
    for (uint64_t i = 0; i < size; i++) {
      if (source_block[i].name == name) {
        return true;
      }
    }

    return false;
  }

  constexpr auto at(const View::Bytes name) const -> const T* {
    for (uint64_t i = 0; i < size; i++) {
      if (source_block[i].name == name) {
        return &source_block[i].data;
      }
    }

    return nullptr;
  }

  constexpr auto at(uint64_t index) const -> const Entry& {
    return source_block[index];
  }

  constexpr auto operator[](const View::Bytes name) const -> const T& {
    return at(name);
  }

  constexpr auto operator[](uint64_t index) const -> const Entry& {
    return at(index);
  }

  constexpr auto get_size() const -> uint64_t { return size; }
  constexpr auto get_data() const -> const Entry* { return source_block; }

 private:
  const Entry* source_block;
  uint64_t size;
};

}  // namespace Perimortem::Memory::View
