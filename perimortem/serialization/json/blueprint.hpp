// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

namespace Perimortem::Serialization::Json {

// Forward declaration — Blueprint stores a pointer to an existing Node for
// the ExistingNode tag; the full definition lives in node.hpp.
class Node;

// Temporary aggregate describing a JSON value tree for Node::construct which
// creates managed Nodes inside of an Arena with appropriate lifetime management
// out of temporary objects.
//
// Stack provided Scalars, Objects and Arrays are all supported but already
// managed objects are currently not supproted. Instead when constructing Nodes
// dynamically build a full Blueprint bottom up instead and provide it to a
// single Node::construct call.
struct Blueprint {
  // Type tags for Blueprint objects which maps almost 1:1 with the types in
  // Node. Compound is the one edge case which could be an Array or and Object
  // with and empty `name` field resulting in an Array.
  enum class Tag : Bits_8 {
    Null,
    String,
    Number,
    Real,
    Flag,
    Node,
    Compound,
  };

  union {
    Core::View::Bytes string_val;
    struct {
      const Blueprint* ptr;
      Count size;
    } compound;
    const Node* node_ptr;
    Signed_64 number_val;
    Real_64 real_val;
    Bool flag_val;
  };
  Core::View::Bytes name;
  Tag tag = Tag::Null;

  // Unnamed scalar constructors.
  Blueprint() : string_val(), name(), tag(Tag::Null) {}
  Blueprint(Core::View::Bytes text)
      : string_val(text), name(), tag(Tag::String) {}
  Blueprint(Bits_8 number) : number_val(number), name(), tag(Tag::Number) {}
  Blueprint(Bits_16 number) : number_val(number), name(), tag(Tag::Number) {}
  Blueprint(Bits_32 number) : number_val(number), name(), tag(Tag::Number) {}
  Blueprint(Bits_64 number) : number_val(number), name(), tag(Tag::Number) {}
  Blueprint(Signed_8 number) : number_val(number), name(), tag(Tag::Number) {}
  Blueprint(Signed_16 number) : number_val(number), name(), tag(Tag::Number) {}
  Blueprint(Signed_32 number) : number_val(number), name(), tag(Tag::Number) {}
  Blueprint(Signed_64 number) : number_val(number), name(), tag(Tag::Number) {}
  Blueprint(Real_64 real) : real_val(real), name(), tag(Tag::Real) {}
  Blueprint(Real_32 real) : Blueprint(Real_64(real)) {}
  Blueprint(Bool flag) : flag_val(flag), name(), tag(Tag::Flag) {}
  Blueprint(const Node& node) : node_ptr(&node), name(), tag(Tag::Node) {}

  // Named scalar constructors (member -> scalar)
  Blueprint(Core::View::Bytes member_name, Core::View::Bytes text)
      : string_val(text), name(member_name), tag(Tag::String) {}
  Blueprint(Core::View::Bytes member_name, Bits_8 number)
      : number_val(number), name(member_name), tag(Tag::Number) {}
  Blueprint(Core::View::Bytes member_name, Bits_16 number)
      : number_val(number), name(member_name), tag(Tag::Number) {}
  Blueprint(Core::View::Bytes member_name, Bits_32 number)
      : number_val(number), name(member_name), tag(Tag::Number) {}
  Blueprint(Core::View::Bytes member_name, Bits_64 number)
      : number_val(number), name(member_name), tag(Tag::Number) {}
  Blueprint(Core::View::Bytes member_name, Signed_8 number)
      : number_val(number), name(member_name), tag(Tag::Number) {}
  Blueprint(Core::View::Bytes member_name, Signed_16 number)
      : number_val(number), name(member_name), tag(Tag::Number) {}
  Blueprint(Core::View::Bytes member_name, Signed_32 number)
      : number_val(number), name(member_name), tag(Tag::Number) {}
  Blueprint(Core::View::Bytes member_name, Signed_64 number)
      : number_val(number), name(member_name), tag(Tag::Number) {}
  Blueprint(Core::View::Bytes member_name, Real_64 real)
      : real_val(real), name(member_name), tag(Tag::Real) {}
  Blueprint(Core::View::Bytes member_name, Real_32 real)
      : Blueprint(member_name, Real_64(real)) {}
  Blueprint(Core::View::Bytes member_name, Bool flag)
      : flag_val(flag), name(member_name), tag(Tag::Flag) {}
  Blueprint(Core::View::Bytes member_name, const Node& node)
      : node_ptr(&node), name(member_name), tag(Tag::Node) {}

  // Named Blueprint arrays are treated as a member entry in a hosting Object
  // that hosts a nested structure which could be an Array or Object.
  template <Count N>
  Blueprint(Core::View::Bytes member_name, const Blueprint (&children)[N])
      : compound{children, N}, name(member_name), tag(Tag::Compound) {}

  // An Object without a named nesting is just a flat Array.
  template <Count N>
  Blueprint(const Blueprint (&children)[N])
      : compound{children, N}, tag(Tag::Compound) {}
};

}  // namespace Perimortem::Serialization::Json
