// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "core/memory/arena.hpp"
#include "core/memory/managed/buffer.hpp"
#include "core/memory/view/bytes.hpp"

#include <bit>
#include <charconv>

namespace Perimortem::Memory::Managed {

// A simple linear string which supports historical views on old data
// as long as the associated Arena is still alive.
class Bytes {
 public:
  static constexpr Count start_capacity = 32;
  static constexpr Count growth_factor = 2;

  Bytes(const Bytes& rhs, Count reserved_capacity = start_capacity)
      : arena(rhs.arena) {
    reset(reserved_capacity);
    proxy(rhs);
  };

  Bytes(Bytes&& rhs) : arena(rhs.arena) {
    // Take ownership and invalidate the old
    size = rhs.size;
    capacity = rhs.capacity;
    rented_block = rhs.rented_block;

    // Does not change reservation counts on the rented block.
    rhs.size = 0;
    rhs.size = 0;
    rhs.rented_block = nullptr;
  };

  Bytes(Arena& arena) : arena(arena) { reset(); }

  constexpr operator View::Bytes() const {
    return View::Bytes(rented_block, size);
  }

  auto reset(Count reserved_capacity = start_capacity) -> void {
    size = 0;
    capacity = reserved_capacity;
    rented_block = reinterpret_cast<char*>(arena.allocate(reserved_capacity));
  }

  auto ensure_room(Count required_bytes) -> void {
    if (size + required_bytes <= capacity) {
      return;
    }

    grow(capacity - (size + required_bytes));
  }

  auto resize(Count new_size) -> void {
    if (new_size > capacity) {
      grow(new_size - capacity);
    }

    size = new_size;
  }

  auto take(Bytes& rhs) -> void {
    // Take ownership and invalidate the old
    size = rhs.size;
    capacity = rhs.capacity;
    rented_block = rhs.rented_block;

    // Does not change reservation counts on the rented block.
    rhs.size = 0;
    rhs.size = 0;
    rhs.rented_block = nullptr;
  }

  auto append(char c) -> void {
    if (size > capacity)
      grow(1);

    rented_block[size++] = c;
  }

  auto append(View::Bytes view) -> void {
    if (size + view.get_size() > capacity)
      grow(view.get_size());

    std::memcpy(rented_block + size, view.get_data(), view.get_size());
    size += view.get_size();
  }

  auto append(uint64_t number) -> void {
    // Largest size required to represent any 64bit number.
    constexpr auto number_range = 22;

    // If we can perform an inpace write then attempt to do so.
    if (size + number_range <= capacity) {
      std::to_chars_result result = std::to_chars(
          rented_block + size, rented_block + size + number_range, number);
      if (result.ec != std::errc()) {
        size += result.ptr - (rented_block + size);
      } else {
        append('0');
      }

      return;
    }

    char working_buffer[number_range] = {};
    std::to_chars_result result =
        std::to_chars(working_buffer, working_buffer + number_range, number);
    append(View::Bytes(working_buffer, result.ptr - working_buffer));
  }

  auto append(int64_t number) -> void {
    // Largest size required to represent any 64bit number.
    constexpr auto number_range = 22;

    // If we can perform an inpace write then attempt to do so.
    if (size + number_range <= capacity) {
      std::to_chars_result result = std::to_chars(
          rented_block + size, rented_block + size + number_range, number);
      if (result.ec != std::errc()) {
        size += result.ptr - (rented_block + size);
      } else {
        append('0');
      }

      return;
    }

    char working_buffer[number_range] = {};
    std::to_chars_result result =
        std::to_chars(working_buffer, working_buffer + number_range, number);
    append(View::Bytes(working_buffer, result.ptr - working_buffer));
  }

  auto append(double number) -> void {
    // Largest size required to represent any 64bit number.
    constexpr auto number_range = 22;

    // If we can perform an inpace write then attempt to do so.
    if (size + number_range > capacity) {
      std::to_chars_result result = std::to_chars(
          rented_block + size, rented_block + size + number_range, number);
      if (result.ec != std::errc()) {
        size += result.ptr - (rented_block + size);
      } else {
        append('0');
      }

      return;
    }

    char working_buffer[number_range] = {};
    std::to_chars_result result =
        std::to_chars(working_buffer, working_buffer + number_range, number);
    append(View::Bytes(working_buffer, result.ptr - working_buffer));
  }

  auto proxy(View::Bytes view) -> void {
    if (view.get_size() > capacity)
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

  constexpr auto get_view() const -> const View::Bytes {
    return View::Bytes(rented_block, size);
  }
  constexpr auto get_size() const -> uint64_t { return size; }
  constexpr auto get_arena() const -> Arena& { return arena; }

  //======================================================================
  // Optimized operations
  //======================================================================

  // Writes bytes directly into the buffer.
  template <typename storage_type,
            std::endian target_order = std::endian::little>
  auto write_at(storage_type data, Count location) -> void {
    static_assert(
        sizeof(storage_type) <= 8,
        "Writing blocks larger than 8 bytes is typically an anti-pattern, "
        "use buffers instead or smaller blocks if you care about endianness.");

    if constexpr (std::endian::native != target_order) {
      data = std::byteswap(data);
    }

    if (location + sizeof(storage_type) >= size) {
      grow((location + sizeof(storage_type)) - size);
      size = location + sizeof(storage_type);
    }

    // When writing bytes we may not be aligned so we'll need to copy the bytes.
    std::memcpy(rented_block + location, &data, sizeof(storage_type));
  }

  // Writes bytes directly into the buffer.
  template <typename storage_type,
            std::endian target_order = std::endian::little>
  auto write(storage_type data) -> void {
    static_assert(
        sizeof(storage_type) <= 8,
        "Writing blocks larger than 8 bytes is typically an anti-pattern, "
        "use buffers instead or smaller blocks if you care about endianness.");

    if constexpr (std::endian::native != target_order) {
      data = std::byteswap(data);
    }

    if (size + sizeof(storage_type) > capacity) {
      grow(sizeof(storage_type));
    }

    // When writing bytes we may not be aligned so we'll need to copy the bytes.
    std::memcpy(rented_block + size, &data, sizeof(storage_type));
    size += sizeof(storage_type);
  }

  // Writes bytes directly into the buffer.
  auto write(const View::Bytes data) -> void {
    if (size + data.get_size() > capacity) {
      grow(capacity - (size + data.get_size()));
    }

    // When writing bytes we may not be aligned so we'll need to copy the bytes.
    std::memcpy(rented_block + size, data.get_data(), data.get_size());
    size += data.get_size();
  }

 private:
  auto grow(Count requested) -> void {
    const auto required = capacity + requested;
    // Attempt to grow by a factor of 2.
    // If that doesn't work than grow to exact size.
    capacity *= growth_factor;
    if (capacity < required) {
      capacity = required;
    }

    auto new_block = reinterpret_cast<char*>(arena.allocate(capacity));

    std::memcpy(new_block, rented_block, size);
    rented_block = new_block;
  }

  Arena& arena;
  char* rented_block;
  Count size;
  Count capacity;
};

}  // namespace Perimortem::Memory::Managed
