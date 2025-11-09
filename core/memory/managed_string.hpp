// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "core/memory/arena.hpp"

#include <cstring>
#include <string_view>

namespace Perimortem::Memory {

// References a managed string, or creates a managed string in a host.
//
// The life time of a managed string is assumed to be less than the string that
// it references, even if they don't both exist in the same host arena.
class ManagedString {
 public:
  ManagedString(const ManagedString&) = default;

  // Convert a possibly non-managed string into a managed string.
  ManagedString(Arena& host, const std::string_view& source) {
    auto buffer = reinterpret_cast<char*>(host.allocate(source.size(), 1));
    std::memcpy(buffer, source.data(), source.size());

    size = source.size();
    rented_block = buffer;
  }

  // The string is already managed.
  ManagedString(const std::string_view& source) {
    size = source.size();
    rented_block = source.data();
  }

  constexpr auto operator==(const ManagedString& rhs) const -> bool {
    return rhs.size == size &&
           std::memcmp(rented_block, rhs.rented_block, size) == 0;
  }

  constexpr auto operator==(const std::string_view& rhs) const -> bool {
    return rhs.size() == size &&
           std::memcmp(rented_block, rhs.data(), size) == 0;
  }

  constexpr auto get_size() const -> uint32_t { return size; };

  constexpr auto get_view() const -> std::string_view {
    return std::string_view(rented_block, size);
  };

 private:
  const char* rented_block;
  uint32_t size;
};

}  // namespace Perimortem::Memory
