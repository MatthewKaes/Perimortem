// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa {

// Attribute consumes source attributes without assigning meaning to them. The
// active ISA or sub-ISA owns the eventual metadata facts.
class Attribute {
 public:
  static auto consume_all(Ttx::Lexical::Cursor& cursor) -> Bool;

 private:
  static auto consume(Ttx::Lexical::Cursor& cursor) -> Bool;
};

}  // namespace Tetrodotoxin::Isa
