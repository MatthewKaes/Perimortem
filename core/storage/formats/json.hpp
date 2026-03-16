// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "core/memory/view/bytes.hpp"
#include "core/memory/view/table.hpp"
#include "core/memory/view/vector.hpp"

#include "core/memory/managed/bytes.hpp"

namespace Perimortem::Storage::Json {

class Node {
 public:
  enum class NodeState : uint32_t {
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
  Node(int64_t value) : value() { set(value); }
  Node(double value) : value() { set(value); }
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

  inline constexpr auto set(int64_t value) -> void {
    this->value.number = value;
    this->state = NodeState::Number;
  }

  inline constexpr auto set(double value) -> void {
    this->value.real = value;
    this->state = NodeState::Real;
  }

  inline constexpr auto set(bool value) -> void {
    this->value.boolean = value;
    this->state = NodeState::Boolean;
  }

  inline constexpr auto set() -> void { state = NodeState::Null; }

  auto null() const -> bool;

  auto at(uint32_t index) const -> const Node;
  auto at(const Memory::View::Bytes name) const -> const Node;

  auto operator[](uint32_t index) const -> const Node;
  auto operator[](const Memory::View::Bytes name) const -> const Node;

  auto contains(const Memory::View::Bytes name) const -> bool;

  auto get_bool() const -> bool;
  auto get_int() const -> int64_t;
  auto get_double() const -> double;
  auto get_string() const -> const Memory::View::Bytes;
  auto get_size() const -> uint64_t;

  auto from_source(Memory::Arena& arena,
                   Memory::View::Bytes source,
                   uint32_t position) -> uint32_t;
  auto format(Memory::Arena& arena) const -> Memory::View::Bytes;

 private:
  auto inplace_format(Memory::Managed::Bytes& output) const -> void;

  // Since we will never use more than the full 32bit range we can pack a set of
  // 12 views which gives us back 4 bytes to store the the state guardian and
  // allows us to fit Nodes into 16 byte making them effectively the same as all
  // other views when it comes to packing them in Arenas.
  class [[gnu::packed]] SquishedVector {
    const Node* source_block;
    uint32_t size;

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
    uint32_t size;

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
    const char* source_block;
    uint32_t size;

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
    int64_t number;
    double real;
    bool boolean;
  };

  NodeValue value;
  NodeState state;
};

static_assert(sizeof(Node) == 16, "Size of Node is required to be 16 bytes.");

}  // namespace Perimortem::Storage::Json
