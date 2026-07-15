// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/managed/vector.hpp"

#include "ttx/attribute.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa::Base {

// Attribute consumes the shared source syntax for metadata facts. Most values
// remain authored byte views whose meaning belongs to the active ISA. Reserved
// Tetrodotoxin attributes are normalized here when their published form is a
// compact value rather than source text. The `abi` value is one example.
class Attribute {
 public:
  static auto consume_all(Ttx::Lexical::Cursor& cursor) -> Bool;
  static auto evaluate_all(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Memory::Managed::Vector<Ttx::Attribute>& attributes) -> Bool;
  static auto append_all(
      Perimortem::Core::View::Vector<Ttx::Attribute> source,
      Perimortem::Memory::Managed::Vector<Ttx::Attribute>& target) -> void;
};

}  // namespace Tetrodotoxin::Isa::Base
