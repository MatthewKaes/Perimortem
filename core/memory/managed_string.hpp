// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "core/memory/arena.hpp"

#include <cstring>
#include <string_view>

namespace Perimortem::Memory {

// Converst a string_view into a a
class ManagedString {
 public:
  ManagedString(const ManagedString&) = default;

  ManagedString(Arena& host, std::string_view source) {
    size = source.size();
    rented_block = reinterpret_cast<char*>(host.allocate(source.size(), 1));
    std::memcpy(rented_block, source.data(), source.size());
  }

  constexpr auto operator==(const ManagedString& rhs) const -> bool {
    return rhs.size == size && std::memcmp(rented_block, rhs.rented_block, size) == 0;
  }

  constexpr auto operator==(const std::string_view& rhs) const -> bool {
    return rhs.size() == size && std::memcmp(rented_block, rhs.data(), size) == 0;
  }

  constexpr auto get_size() const -> uint32_t { return size; };

  constexpr auto get_view() const -> std::string_view {
    return std::string_view(rented_block, size);
  };

 private:
  char* rented_block;
  uint32_t size;
};

}  // namespace Perimortem::Memory
