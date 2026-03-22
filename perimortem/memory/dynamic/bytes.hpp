// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/resizable.hpp"
#include "perimortem/memory/const/standard_types.hpp"
#include "perimortem/memory/view/bytes.hpp"

#include <charconv>
#include <cstring>

namespace Perimortem::Memory::Dynamic {

// A vector of dynamically managed bytes with value semantics.
// For a buffer of bytes with reference symantics use Record.
class Bytes : public Allocator::Resizable<Byte, 32> {
 public:
  Bytes(Count reserved_capacity = min_capacity())
      : Resizable(reserved_capacity) {}

  Bytes(const Bytes& rhs)
      : Resizable(std::max(rhs.get_size(), min_capacity())),
        size(rhs.get_size()) {
    // Bytes don't require any special handling so just memcpy.
    std::memcpy(access(), rhs.access(), size);
  };

  Bytes(Bytes&& rhs) : Resizable(std::move(rhs)) {};

  auto append(Byte b) -> void {
    set_capacity(size + 1);
    access()[size++] = b;
  }

  auto append(View::Bytes view) -> void {
    set_capacity(size + view.get_size());

    std::memcpy(access() + size, view.get_data(), view.get_size());
    size += view.get_size();
  }

  auto append(Count number) -> void {
    // Largest size required to represent any 64bit number.
    constexpr auto number_range = 22;
    char working_buffer[number_range] = {};
    std::to_chars_result result =
        std::to_chars(working_buffer, working_buffer + number_range, number);

    if (result.ec != std::errc()) {
      append(View::Bytes(working_buffer, result.ptr - working_buffer));
    } else {
      append('0'_byte);
    }
  }

  auto append(Long number) -> void {
    // Largest size required to represent any 64bit number.
    constexpr auto number_range = 22;
    char working_buffer[number_range] = {};
    std::to_chars_result result =
        std::to_chars(working_buffer, working_buffer + number_range, number);

    if (result.ec != std::errc()) {
      append(View::Bytes(working_buffer, result.ptr - working_buffer));
    } else {
      append('0'_byte);
    }
  }

  auto append(Real_64 number) -> void {
    // Largest size required to represent any 64bit number.
    constexpr auto number_range = 28;
    char working_buffer[number_range] = {};
    std::to_chars_result result =
        std::to_chars(working_buffer, working_buffer + number_range, number);

    if (result.ec != std::errc()) {
      append(View::Bytes(working_buffer, result.ptr - working_buffer));
    } else {
      append('0'_byte);
    }
  }

  auto proxy(View::Bytes view) -> void {
    const auto original_end = size;
    set_capacity(view.get_size());

    std::memcpy(access(), view.get_data(), view.get_size());
    size = view.get_size();
  }

  auto convert(Byte source, Byte target) -> void {
    for (int i = 0; i < size; i++) {
      if (access()[i] == source) {
        access()[i] = target;
      }
    }
  }

  auto operator[](Count index) const -> char {
    if (index > size)
      return 0;

    return access()[index];
  }

  auto at(Count index) const -> char {
    if (index > size)
      return 0;

    return access()[index];
  }

  constexpr auto get_size() const -> Count { return size; }

  constexpr auto get_view() const -> const View::Bytes {
    return View::Bytes(access(), size);
  }

  constexpr operator View::Bytes() const { return get_view(); }

 private:
  Count size = 0;
};

}  // namespace Perimortem::Memory::Dynamic
