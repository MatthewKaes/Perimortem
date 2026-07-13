// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/managed/vector.hpp"

#include "ttx/attribute.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa::Base {

// Attribute consumes source attributes without assigning meaning to them. The
// active ISA or sub-ISA owns the eventual metadata facts.
class Attribute {
 public:
  static auto consume_all(Ttx::Lexical::Cursor& cursor) -> Bool;
  static auto evaluate_all(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Memory::Managed::Vector<Ttx::Attribute>& attributes) -> Bool;
  static auto append_all(
      Perimortem::Core::View::Vector<Ttx::Attribute> source,
      Perimortem::Memory::Managed::Vector<Ttx::Attribute>& target) -> void;

 private:
  static auto key(Perimortem::Core::View::Bytes source)
      -> Perimortem::Core::View::Bytes;
  static auto consume_arguments(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Core::View::Bytes& value) -> Bool;
  static auto consume(Ttx::Lexical::Cursor& cursor) -> Bool;
  static auto evaluate(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Memory::Managed::Vector<Ttx::Attribute>& attributes) -> Bool;
};

}  // namespace Tetrodotoxin::Isa::Base
