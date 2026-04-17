// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/view/bytes.hpp"
#include "perimortem/memory/view/table.hpp"
#include "perimortem/memory/view/vector.hpp"

#include "perimortem/memory/managed/bytes.hpp"

namespace Perimortem::Storage::Json {

class Node {
 public:
  enum class NodeState : Bits_32 {
    Null,
    Raw,
    String,
    Number,
    Real,
    Object,
    Array,
    Boolean
  };

  Node() : value() {}
  Node(const Node& rhs) : value(rhs.value) {}
  Node(const Memory::View::Bytes value) : value() { set(value); }
  Node(const Memory::View::Table<Node> value) : value() { set(value); }
  Node(const Memory::View::Vector<Node> value) : value() { set(value); }
  Node(Long value) : value() { set(value); }
  Node(Real_64 value) : value() { set(value); }
  Node(bool value) : value() { set(value); }

  inline static auto raw(const Memory::View::Bytes value) -> Node {
    Node node;
    node.value.string = value;
    node.state = NodeState::Raw;
    return node;
  }

  inline constexpr auto set(const Memory::View::Bytes value) -> void {
    this->value.string = value;
    this->state = NodeState::String;
  }

  inline constexpr auto set(const Memory::View::Vector<Node> value) -> void {
    this->value.array = value;
    this->state = NodeState::Array;
  }

  inline constexpr auto set(const Memory::View::Table<Node> value) -> void {
    this->value.object = value;
    this->state = NodeState::Object;
  }

  inline constexpr auto set(Long value) -> void {
    this->value.number = value;
    this->state = NodeState::Number;
  }

  inline constexpr auto set(Real_64 value) -> void {
    this->value.real = value;
    this->state = NodeState::Real;
  }

  inline constexpr auto set(bool value) -> void {
    this->value.boolean = value;
    this->state = NodeState::Boolean;
  }

  inline constexpr auto set() -> void { state = NodeState::Null; }

  auto null() const -> bool;

  auto at(Bits_32 index) const -> const Node;
  auto at(const Memory::View::Bytes name) const -> const Node;

  auto operator[](Bits_32 index) const -> const Node;
  auto operator[](const Memory::View::Bytes name) const -> const Node;

  auto contains(const Memory::View::Bytes name) const -> bool;

  auto get_bool() const -> bool;
  auto get_int() const -> Count;
  auto get_double() const -> double;
  auto get_string() const -> const Memory::View::Bytes;
  auto get_size() const -> Count;

  auto from_source(Memory::Allocator::Arena& arena,
                   Memory::View::Bytes source,
                   Bits_32 position) -> Bits_32;
  auto format(Memory::Allocator::Arena& arena) const -> Memory::View::Bytes;

 private:
  auto serialized_size() const -> Count;
  auto inplace_format(Memory::Managed::Bytes& output) const -> void;

  // Since we will never use more than the full 32bit range we can pack a set of
  // 12 views which gives us back 4 bytes to store the the state guardian and
  // allows us to fit Nodes into 16 byte making them effectively the same as all
  // other views when it comes to packing them in Arenas.
  class [[gnu::packed]] SquishedVector {
    const Node* source_block;
    Bits_32 size;

   public:
    SquishedVector& operator=(const Memory::View::Vector<Node>& rhs) {
      source_block = rhs.get_data();
      size = rhs.get_size();
      return *this;
    }

    constexpr operator Memory::View::Vector<Node>() const {
      return Memory::View::Vector<Node>(source_block, size);
    }
  };

  class [[gnu::packed]] SquishedTable {
    using Entry = Memory::View::Table<Node>::Entry;
    const Entry* source_block;
    Bits_32 size;

   public:
    SquishedTable& operator=(const Memory::View::Table<Node>& rhs) {
      source_block = rhs.get_data();
      size = rhs.get_size();
      return *this;
    }

    constexpr operator Memory::View::Table<Node>() const {
      return Memory::View::Table<Node>(source_block, size);
    }
  };

  class [[gnu::packed]] SquishedBytes {
    const Byte* source_block;
    Bits_32 size;

   public:
    SquishedBytes& operator=(const Memory::View::Bytes& rhs) {
      source_block = rhs.get_data();
      size = rhs.get_size();
      return *this;
    }

    constexpr operator Memory::View::Bytes() const {
      return Memory::View::Bytes(source_block, size);
    }
  };

  union [[gnu::packed]] NodeValue {
    NodeValue() {};
    SquishedVector array;
    SquishedTable object;
    SquishedBytes string;
    Long number;
    Real_64 real;
    bool boolean;
  };

  NodeValue value;
  NodeState state;
};

static_assert(sizeof(Node) == 16, "Size of Node is required to be 16 bytes.");

}  // namespace Perimortem::Storage::Json
