// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "memory/arena.hpp"
#include "memory/byte_view.hpp"

namespace Perimortem::Memory {

// A simple linear string which supports historical views on old data
// as long as the associated Arena is still alive.
class ManagedString {
 public:
  static constexpr uint32_t start_capacity = 32;
  static constexpr uint32_t growth_factor = 2;

  ManagedString(const ManagedString& rhs) {
    reset();
    proxy(rhs);
  };

  ManagedString(ManagedString&& rhs) {
    // Take ownership and invalidate the old
    size = rhs.size;
    capacity = rhs.capacity;
    rented_block = rhs.rented_block;

    // Does not change reservation counts on the rented block.
    rhs.size = 0;
    rhs.size = 0;
    rhs.rented_block = nullptr;
  };

  ManagedString(Arena& arena) : arena(&arena) { reset(); }

  operator ByteView() const { return ByteView(rented_block, size); }

  auto reset() -> void {
    size = 0;
    capacity = start_capacity;
    rented_block = reinterpret_cast<char*>(arena->allocate(start_capacity));
  }

  auto append(char c) -> void {
    if (size == capacity)
      grow(1);

    rented_block[size++] = c;
  }

  auto append(ByteView view) -> void {
    if (size + view.get_size() >= capacity)
      grow(view.get_size());

    std::memcpy(rented_block + size, view.get_data(), view.get_size());
    size += view.get_size();
  }

  auto proxy(ByteView view) -> void {
    if (view.get_size() >= capacity)
      grow(view.get_size());

    std::memcpy(rented_block + size, view.get_data(), view.get_size());
    size = view.get_size();
  }

  auto convert(char source, char target) -> void {
    for (int i = 0; i < size; i++) {
      if (rented_block[i] == source) {
        rented_block[i] = target;
      }
    }
  }

  auto operator[](uint32_t index) const -> char {
    if (index > size)
      return 0;

    return rented_block[index];
  }

  auto at(uint32_t index) const -> char {
    if (index > size)
      return 0;

    return rented_block[index];
  }

  constexpr auto get_size() const -> uint32_t { return size; };

 private:
  auto grow(uint32_t requested) -> void {
    // Attempt to grow by a factor of 2.
    // If that doesn't work than grow to exact size.
    capacity *= growth_factor;
    if (capacity < capacity + requested) {
      capacity = requested;
    }

    auto new_block = reinterpret_cast<char*>(arena->allocate(capacity));

    std::memcpy(new_block, rented_block, size);
    rented_block = new_block;
  }

  Arena* arena;
  char* rented_block;
  uint32_t size;
  uint32_t capacity;
};

}  // namespace Perimortem::Memory
