// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "memory/managed_lookup.hpp"
#include "memory/managed_string.hpp"
#include "memory/managed_vector.hpp"

#include <variant>

namespace Perimortem::Storage::Json {

class Node {
 public:
  Node() : value(0) {}

  inline constexpr auto set(const Memory::ManagedString& value) -> void {
    this->value = value;
  }
  inline constexpr auto set(const Memory::ManagedVector<Node*>& value) -> void {
    this->value = value;
  }
  inline constexpr auto set(const Memory::ManagedLookup<Node>& value) -> void {
    this->value = value;
  }
  inline constexpr auto set(long value) -> void { this->value = value; }
  inline constexpr auto set(double value) -> void { this->value = value; }
  inline constexpr auto set(bool value) -> void { this->value = value; }

  auto null() const -> bool;

  auto at(uint32_t index) const -> const Node*;
  auto at(const std::string_view& name) const -> const Node*;

  auto operator[](uint32_t index) const -> const Node*;
  auto operator[](const std::string_view& name) const -> const Node*;

  auto contains(const std::string_view& name) const -> bool;

  auto get_bool() const -> const bool;
  auto get_int() const -> const long;
  auto get_double() const -> const double;
  auto get_string() const -> const Memory::ManagedString;

 private:
  std::variant<Memory::ManagedString,
               Memory::ManagedVector<Node*>,
               Memory::ManagedLookup<Node>,
               bool,
               long,
               double>
      value;
};

auto parse(Memory::Arena& arena,
           Perimortem::Memory::ManagedString source,
           uint32_t& position) -> Node*;

}  // namespace Perimortem::Storage::Json
