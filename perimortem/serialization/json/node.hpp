// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/serialization/json/blueprint.hpp"

namespace Perimortem::Serialization::Json {

struct Member;

class Node {
 public:
  Node() : data{.ptr = nullptr, .size = 0, .state = 0} { set(); }
  Node(const Node& rhs) : data(rhs.data) {};
  Node(const Core::View::Bytes value)
      : data{.ptr = nullptr, .size = 0, .state = 0} {
    set(value);
  }
  Node(const Core::View::Vector<Node> value)
      : data{.ptr = nullptr, .size = 0, .state = 0} {
    set(value);
  }
  Node(const Core::View::Vector<Member> value)
      : data{.ptr = nullptr, .size = 0, .state = 0} {
    set(value);
  }
  Node(Signed_64 value) : data{.ptr = nullptr, .size = 0, .state = 0} {
    set(value);
  }
  Node(Real_32 value) : data{.ptr = nullptr, .size = 0, .state = 0} {
    set(Real_64(value));
  }
  Node(Real_64 value) : data{.ptr = nullptr, .size = 0, .state = 0} {
    set(value);
  }
  Node(Bool value) : data{.ptr = nullptr, .size = 0, .state = 0} { set(value); }

  auto set(const Core::View::Bytes value) -> void;
  auto set(const Core::View::Vector<Node> value) -> void;
  auto set(const Core::View::Vector<Member> value) -> void;
  auto set(Signed_64 value) -> void;
  auto set(Real_64 value) -> void;
  auto set(Bool value) -> void;
  auto set() -> void;

  auto at(Bits_32 index) const -> const Node;
  auto at(const Core::View::Bytes name) const -> const Node;

  auto operator[](Bits_32 index) const -> const Node;
  auto operator[](const Core::View::Bytes name) const -> const Node;

  auto contains(const Core::View::Bytes name) const -> Bool;

  auto get_flag() const -> Bool;
  auto get_number() const -> Signed_64;
  auto get_real() const -> double;
  auto get_string() const -> const Core::View::Bytes;
  auto get_array() const -> const Core::View::Vector<Node>;
  auto get_object() const -> const Core::View::Vector<Member>;

  // Gets the size in elements of any range type (string, array, object)
  // Returns 0 for any base types (null, flag, number, real)
  auto get_size() const -> Count;

  auto is_null() const -> Bool;
  auto is_flag() const -> Bool;
  auto is_number() const -> Bool;
  auto is_real() const -> Bool;
  auto is_string() const -> Bool;
  auto is_array() const -> Bool;
  auto is_object() const -> Bool;

  // Recursively materialises a Blueprint array into an arena-backed object
  // node. Children with names become an object; children without names become
  // an array. All stack C-arrays live for the full construct() expression.
  template <Count N>
  static auto construct(
      Memory::Allocator::Arena& arena,
      const Blueprint (&entries)[N]) -> Node {
    return construct(arena, entries, N);
  }

  static auto construct(
      Memory::Allocator::Arena& arena,
      const Json::Blueprint* entries,
      Count count) -> Node;

  static auto construct(
      Memory::Allocator::Arena& arena,
      const Json::Blueprint& root) -> Node;

  auto parse(
      Memory::Allocator::Arena& arena,
      Core::View::Bytes source,
      Count position = 0) -> Count;
  auto format(Memory::Allocator::Arena& arena) const -> Core::View::Bytes;

 private:
  auto serialized_size() const -> Count;

  struct {
    union {
      const void* ptr;
      Signed_64 number;
      Real_64 real;
      Bool flag;
    };
    Bits_32 size;
    Bits_32 state;
  } data;
};

static_assert(sizeof(Node) == 16, "Size of Node is required to be 16 bytes.");

struct Member {
  const Core::View::Bytes name;
  const Node node;
};

using Object = Core::View::Vector<Json::Member>;
using Array = Core::View::Vector<Node>;

}  // namespace Perimortem::Serialization::Json
