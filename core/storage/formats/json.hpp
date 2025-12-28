// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "memory/byte_view.hpp"
#include "memory/managed_lookup.hpp"
#include "memory/managed_string.hpp"
#include "memory/managed_vector.hpp"

#include <variant>

namespace Perimortem::Storage::Json {

class Node {
 public:
  enum class NodeState { Null };

  Node() : value(NodeState::Null) {}

  inline constexpr auto set(const Memory::ByteView& value) -> void {
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
  inline constexpr auto set(NodeState value) -> void { this->value = value; }

  auto null() const -> bool;

  auto at(uint32_t index) const -> const Node&;
  auto at(const std::string_view& name) const -> const Node&;

  auto operator[](uint32_t index) const -> const Node&;
  auto operator[](const std::string_view& name) const -> const Node&;

  auto contains(const std::string_view& name) const -> bool;

  auto get_bool() const -> const bool;
  auto get_int() const -> const long;
  auto get_double() const -> const double;
  auto get_string() const -> const Memory::ByteView;


  // Treats this node as an array and adds a node to it.
  // If the node isn't an array already it will convert to an empty array and
  // drop any prior data.
  // WARNING: Switching between types leaves the any old data in the arena
  // resulting in a "memory" leak, so it's best to avoid rapid switching.
  auto extend(Memory::Arena& arena, Node& node) -> void;
  // Treats this node as an object and adds a key / value pair to it.
  // If the node isn't an object already it will convert to an empty array and
  // drop any prior data.
  // WARNING: Switching between types leaves the any old data in the arena
  // resulting in a "memory" leak, so it's best to avoid rapid switching.
  auto extend(Memory::Arena& arena,
              Perimortem::Memory::ByteView name,
              Node& node) -> void;

  auto from_source(Memory::Arena& arena,
                   Perimortem::Memory::ByteView source,
                   uint32_t position) -> uint32_t;
  auto format(Memory::Arena& arena) -> Memory::ManagedString;

 private:
  auto inplace_format(Memory::ManagedString& arena) -> void;
  std::variant<Memory::ByteView,
               Memory::ManagedVector<Node*>,
               Memory::ManagedLookup<Node>,
               bool,
               long,
               double,
               NodeState>
      value;
};

}  // namespace Perimortem::Storage::Json
