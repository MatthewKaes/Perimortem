// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/amorphous.hpp"
#include "perimortem/core/view/structured.hpp"
#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/serialization/textual/stream.hpp"

namespace Perimortem::Serialization::Json {

struct Member;

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
  Node(const Core::View::Amorphous value) : value() { set(value); }
  Node(const Core::View::Structured<Node> value) : value() { set(value); }
  Node(const Core::View::Structured<Member> value) : value() { set(value); }
  Node(Long value) : value() { set(value); }
  Node(Real_64 value) : value() { set(value); }
  Node(Bool value) : value() { set(value); }

  inline static auto raw(const Core::View::Amorphous value) -> Node {
    Node node;
    node.value.string = value;
    node.state = NodeState::Raw;
    return node;
  }

  inline constexpr auto set(const Core::View::Amorphous value) -> void {
    this->value.string = value;
    this->state = NodeState::String;
  }

  inline constexpr auto set(const Core::View::Structured<Node> value)
      -> void {
    this->value.array = value;
    this->state = NodeState::Array;
  }

  inline constexpr auto set(const Core::View::Structured<Member> value) -> void {
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

  inline constexpr auto set(Bool value) -> void {
    this->value.boolean = value;
    this->state = NodeState::Boolean;
  }

  inline constexpr auto set() -> void { state = NodeState::Null; }

  auto null() const -> Bool;

  auto at(Bits_32 index) const -> const Node;
  auto at(const Core::View::Amorphous name) const -> const Node;

  auto operator[](Bits_32 index) const -> const Node;
  auto operator[](const Core::View::Amorphous name) const -> const Node;

  auto contains(const Core::View::Amorphous name) const -> Bool;

  auto get_bool() const -> Bool;
  auto get_int() const -> Count;
  auto get_double() const -> double;
  auto get_string() const -> const Core::View::Amorphous;
  auto get_size() const -> Count;

  auto from_source(Memory::Allocator::Arena& arena,
                   Core::View::Amorphous source,
                   Count position = 0) -> Count;
  auto format(Memory::Allocator::Arena& arena) const
      -> Core::View::Amorphous;

 private:
  auto serialized_size() const -> Count;
  auto inplace_format(Textual::Stream& output) const -> void;

  // Since we will never use more than the full 32bit range we can pack a set of
  // 12 views which gives us back 4 bytes to store the the state guardian and
  // allows us to fit Nodes into 16 byte making them effectively the same as all
  // other views when it comes to packing them in Arenas.
  class [[gnu::packed]] SquishedVector {
    const Node* source_block;
    Bits_32 size;

   public:
    SquishedVector& operator=(const Core::View::Structured<Node>& rhs) {
      source_block = rhs.get_data();
      size = rhs.get_size();
      return *this;
    }

    constexpr operator Core::View::Structured<Node>() const {
      return Core::View::Structured<Node>(source_block, size);
    }
  };

  class [[gnu::packed]] SquishedTable {
    const Member* source_block;
    Bits_32 size;

   public:
    SquishedTable& operator=(const Core::View::Structured<Member>& rhs) {
      source_block = rhs.get_data();
      size = rhs.get_size();
      return *this;
    }

    constexpr operator Core::View::Structured<Member>() const {
      return Core::View::Structured<Member>(source_block, size);
    }
  };

  class [[gnu::packed]] SquishedBytes {
    const Byte* source_block;
    Bits_32 size;

   public:
    SquishedBytes& operator=(const Core::View::Amorphous& rhs) {
      source_block = rhs.get_data();
      size = rhs.get_size();
      return *this;
    }

    constexpr operator Core::View::Amorphous() const {
      return Core::View::Amorphous(source_block, size);
    }
  };

  union [[gnu::packed]] NodeValue {
    NodeValue() {};
    SquishedVector array;
    SquishedTable object;
    SquishedBytes string;
    Long number;
    Real_64 real;
    Bool boolean;
  };

  NodeValue value;
  NodeState state;
};

static_assert(sizeof(Node) == 16, "Size of Node is required to be 16 bytes.");

struct Member {
  const Core::View::Amorphous name;
  const Node node;
};

}  // namespace Perimortem::Serialization::Json
