// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

namespace Perimortem::Serialization::Json {

struct Member;

class Node {
 public:
#define DEFAULT_DATA data{.ptr = nullptr, .size = 0, .state = 0}

  Node() : DEFAULT_DATA { set(); }
  Node(const Node& rhs) : data(rhs.data) {};
  Node(const Core::View::Bytes value) : DEFAULT_DATA { set(value); }
  Node(const Core::View::Vector<Node> value) : DEFAULT_DATA { set(value); }
  Node(const Core::View::Vector<Member> value) : DEFAULT_DATA { set(value); }
  Node(Long value) : DEFAULT_DATA { set(value); }
  Node(Real_32 value) : DEFAULT_DATA { set(Real_64(value)); }
  Node(Real_64 value) : DEFAULT_DATA { set(value); }
  Node(Bool value) : DEFAULT_DATA { set(value); }

#undef DEFAULT_DATA

  auto set(const Core::View::Bytes value) -> void;
  auto set(const Core::View::Vector<Node> value) -> void;
  auto set(const Core::View::Vector<Member> value) -> void;
  auto set(Long value) -> void;
  auto set(Real_64 value) -> void;
  auto set(Bool value) -> void;
  auto set() -> void;

  auto at(Bits_32 index) const -> const Node;
  auto at(const Core::View::Bytes name) const -> const Node;

  auto operator[](Bits_32 index) const -> const Node;
  auto operator[](const Core::View::Bytes name) const -> const Node;

  auto contains(const Core::View::Bytes name) const -> Bool;

  auto get_flag() const -> Bool;
  auto get_number() const -> Count;
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
      Long number;
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
