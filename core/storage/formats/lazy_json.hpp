// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "memory/managed_string.hpp"

#include <array>
#include <cstdint>

namespace Perimortem::Storage::Json {

template <typename LazyNode>
class LazyMember {
 public:
  LazyMember() {};
  LazyMember(const LazyNode name, const LazyNode value)
      : name(name), value(value) {}

  LazyNode name;
  LazyNode value;
};

template <typename LazyMember, typename LazyNode, uint32_t max_items = 16>
class LazyObject {
 public:
  LazyMember members[max_items + 1] = {};

  inline constexpr auto contains(const std::string_view& name) const -> bool {
    return contains(Perimortem::Memory::ManagedString(name));
  }

  inline constexpr auto contains(
      const Perimortem::Memory::ManagedString& name) const -> bool {
    for (uint32_t i = 0; i < max_items; i++) {
      if (members[i].name.get_view() == name) {
        return true;
      }
    }

    return false;
  }

  inline constexpr auto operator[](const std::string_view& name) const
      -> const LazyNode& {
    return operator[](Perimortem::Memory::ManagedString(name));
  }

  inline constexpr auto operator[](
      const Perimortem::Memory::ManagedString& name) const -> const LazyNode& {
    for (uint32_t i = 0; i < max_items; i++) {
      if (members[i].name.get_view() == name) {
        return members[i].value;
      }
    }

    // Default empty value
    return members[max_items].value;
  }
};

class LazyNode {
 public:
  LazyNode() : sub_section() {}
  LazyNode(const Perimortem::Memory::ManagedString& contents)
      : sub_section(contents) {}

  auto get_bool() const -> bool;
  auto get_int() const -> long;
  auto get_double() const -> double;
  auto get_string() const -> const Perimortem::Memory::ManagedString;

  template <uint32_t max_items = 16>
  constexpr auto get_array() const -> const std::array<LazyNode, max_items> {
    std::array<LazyNode, max_items> values;
    uint32_t value_count = 0;
    uint32_t position = 0;

    if (sub_section.empty() || sub_section[position] != '[')
      return values;

    return values;
  }

  template <uint32_t max_items = 16>
  constexpr auto get_object() const -> const LazyObject<LazyMember<LazyNode>, LazyNode, max_items> {
    LazyObject<LazyMember<LazyNode>, LazyNode, max_items> object;
    uint32_t value_count = 0;
    uint32_t position = 0;

    if (sub_section.empty() || sub_section[position] != '{')
      return object;

    position++;
    while (value_count < max_items && position < sub_section.get_size() - 1) {
      object.members[value_count].name = parse_section(position);
      object.members[value_count].value = parse_section(position);
      value_count++;
    }

    return object;
  }

  constexpr inline auto get_view() const
      -> const Perimortem::Memory::ManagedString& {
    return sub_section;
  }

 private:
  auto parse_section(uint32_t& position) const -> const LazyNode;

  Perimortem::Memory::ManagedString sub_section;
};

}  // namespace Perimortem::Storage::Json
